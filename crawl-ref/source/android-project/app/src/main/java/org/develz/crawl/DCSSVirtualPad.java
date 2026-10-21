package org.develz.crawl;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.SparseIntArray;
import android.view.MotionEvent;
import android.view.View;

import org.libsdl.app.SDLActivity;

/**
 * 日本語版: 画面に重ねるバーチャルパッド。押したボタンは物理のパッドと同じ口
 * (SDLActivity.nativePadButton / nativePadAxis)で C++ のゲームパッドの処理へ渡す。
 *
 * 配置と大きさは、ユーザーの変愚蛮怒 HD2D のバーチャルパッド(hd2d/ui/virtual_pad.cpp の
 * kDefaultsLand / kDefaultsPort。実機で詰めた比率)を借りた。右スティックはパッドの
 * カーソルを動かし、L3・R3 はその位置での左・右クリック(Windows 版と同じ)。
 *
 * ボタンの外に触れたときは、触れたのを受け取らずに下の画面(SDLSurface)へ素通りさせる。
 * 設計は ja-tools/GAMEPAD_DESIGN.md §9。
 */
public class DCSSVirtualPad extends View {

    // 形: 0 = 円、1 = 角丸の長方形、2 = スティック
    private static final int CIRCLE = 0, RECT = 1, STICK = 2;

    // 並び: A B X Y RB RT LB LT Start Back 左スティック R3 L3 右スティック
    private static final int A = 0, B = 1, X = 2, Y = 3, RB = 4, RT = 5, LB = 6, LT = 7,
                             START = 8, BACK = 9, LS = 10, R3 = 11, L3 = 12, RS = 13,
                             COUNT = 14;
    private static final String[] LABELS =
        { "A", "B", "X", "Y", "RB", "RT", "LB", "LT", "Start", "Back", "", "R3", "L3", "" };

    // { 中心 x(幅比), 中心 y(高さ比), 形, 半径か半幅, 半高かつまみの半径 }(大きさは sizeBasis 比)
    private static final float[][] LAND = {
        { 0.673f, 0.892f, CIRCLE, 0.085f, 0f },
        { 0.660f, 0.670f, CIRCLE, 0.079f, 0f },
        { 0.713f, 0.441f, CIRCLE, 0.084f, 0f },
        { 0.799f, 0.313f, CIRCLE, 0.082f, 0f },
        { 0.917f, 0.098f, RECT, 0.186f, 0.065f },
        { 0.744f, 0.150f, RECT, 0.093f, 0.031f },
        { 0.057f, 0.098f, RECT, 0.186f, 0.065f },
        { 0.231f, 0.153f, RECT, 0.093f, 0.031f },
        { 0.504f, 0.870f, RECT, 0.060f, 0.028f },
        { 0.374f, 0.868f, RECT, 0.060f, 0.028f },
        { 0.164f, 0.711f, STICK, 0.262f, 0.100f },
        { 0.911f, 0.316f, CIRCLE, 0.073f, 0f },
        { 0.079f, 0.292f, CIRCLE, 0.068f, 0f },
        { 0.840f, 0.712f, STICK, 0.262f, 0.100f },
    };
    private static final float[][] PORT = {
        { 0.519f, 0.924f, CIRCLE, 0.043f, 0f },
        { 0.527f, 0.839f, CIRCLE, 0.043f, 0f },
        { 0.608f, 0.767f, CIRCLE, 0.043f, 0f },
        { 0.765f, 0.738f, CIRCLE, 0.043f, 0f },
        { 0.857f, 0.557f, RECT, 0.117f, 0.046f },
        { 0.626f, 0.579f, RECT, 0.068f, 0.037f },
        { 0.143f, 0.559f, RECT, 0.117f, 0.046f },
        { 0.381f, 0.578f, RECT, 0.069f, 0.039f },
        { 0.500f, 0.672f, RECT, 0.060f, 0.028f },
        { 0.360f, 0.672f, RECT, 0.060f, 0.028f },
        { 0.202f, 0.863f, STICK, 0.136f, 0.073f },
        { 0.907f, 0.665f, CIRCLE, 0.043f, 0f },
        { 0.096f, 0.675f, CIRCLE, 0.043f, 0f },
        { 0.772f, 0.865f, STICK, 0.136f, 0.073f },
    };
    private static final float TUNED_ASPECT = 2.36f, TUNED_ASPECT_PORTRAIT = 1.78f;

    // スティックの向き: 半径のこの割合を超えたら入り、下回ったら中立(ヒステリシス)
    private static final float STICK_ENTER = 0.34f, STICK_LEAVE = 0.22f;
    private static final float OCTANT_SIN = 0.38268343f; // sin(22.5°)
    // 右スティック: 半径のこの割合までは中立。外側は C++ の死区(8000)の外へ写す
    private static final float RS_DEAD = 0.30f;
    private static final int CURSOR_DEADZONE = 8000;

    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF rect = new RectF();

    private final SparseIntArray pointerControl = new SparseIntArray(); // 指 → 掴んだボタン
    private final boolean[] pressed = new boolean[COUNT];
    private int stickDx = 0, stickDy = 0;
    private final float[] knobX = new float[COUNT], knobY = new float[COUNT];

    public DCSSVirtualPad(Context context) {
        super(context);
        setFocusable(false);
        line.setStyle(Paint.Style.STROKE);
        text.setTextAlign(Paint.Align.CENTER);
        text.setFakeBoldText(true);
    }

    private boolean portrait() {
        return getHeight() > getWidth();
    }

    private float sizeBasis() {
        float w = Math.max(1, getWidth()), h = Math.max(1, getHeight());
        return portrait() ? Math.min(w, h / TUNED_ASPECT_PORTRAIT)
                          : Math.min(h, w / TUNED_ASPECT);
    }

    private float[] spec(int c) {
        return portrait() ? PORT[c] : LAND[c];
    }

    private float cx(int c) { return spec(c)[0] * getWidth(); }
    private float cy(int c) { return spec(c)[1] * getHeight(); }
    private float sa(int c) { return spec(c)[3] * sizeBasis(); }
    private float sb(int c) { return spec(c)[4] * sizeBasis(); }

    private int hit(float x, float y) {
        // スティックは少し広めに取る(指が外へずれても掴める)
        for (int c = 0; c < COUNT; c++) {
            float dx = x - cx(c), dy = y - cy(c);
            switch ((int) spec(c)[2]) {
            case RECT:
                if (Math.abs(dx) <= sa(c) && Math.abs(dy) <= sb(c) * 1.4f)
                    return c;
                break;
            case STICK:
                if (dx * dx + dy * dy <= sa(c) * sa(c) * 1.2f)
                    return c;
                break;
            default:
                if (dx * dx + dy * dy <= sa(c) * sa(c) * 1.15f)
                    return c;
            }
        }
        return -1;
    }

    private static int buttonOf(int c) {
        switch (c) {
        case A: return DCSSGamepad.BUTTON_A;
        case B: return DCSSGamepad.BUTTON_B;
        case X: return DCSSGamepad.BUTTON_X;
        case Y: return DCSSGamepad.BUTTON_Y;
        case RB: return DCSSGamepad.BUTTON_RIGHTSHOULDER;
        case LB: return DCSSGamepad.BUTTON_LEFTSHOULDER;
        case START: return DCSSGamepad.BUTTON_START;
        case BACK: return DCSSGamepad.BUTTON_BACK;
        case L3: return DCSSGamepad.BUTTON_LEFTSTICK;
        case R3: return DCSSGamepad.BUTTON_RIGHTSTICK;
        default: return -1;
        }
    }

    private void press(int c, boolean down) {
        if (pressed[c] == down)
            return;
        pressed[c] = down;
        if (c == LT || c == RT) {
            SDLActivity.nativePadAxis(c == LT ? DCSSGamepad.AXIS_TRIGGERLEFT
                                              : DCSSGamepad.AXIS_TRIGGERRIGHT,
                                      down ? 32767 : 0);
        } else if (c != LS && c != RS) {
            SDLActivity.nativePadButton(buttonOf(c), down);
        }
    }

    private void setStick(int dx, int dy) {
        if (dx == stickDx && dy == stickDy)
            return;
        // 先に離してから押す(状態機械は十字の 4 つのボタンを見て向きを決める)
        if (stickDx < 0 && dx >= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_LEFT, false);
        if (stickDx > 0 && dx <= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_RIGHT, false);
        if (stickDy < 0 && dy >= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_UP, false);
        if (stickDy > 0 && dy <= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_DOWN, false);
        if (dx < 0 && stickDx >= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_LEFT, true);
        if (dx > 0 && stickDx <= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_RIGHT, true);
        if (dy < 0 && stickDy >= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_UP, true);
        if (dy > 0 && stickDy <= 0) SDLActivity.nativePadButton(DCSSGamepad.BUTTON_DPAD_DOWN, true);
        stickDx = dx;
        stickDy = dy;
    }

    private void moveStick(int c, float x, float y) {
        float dx = x - cx(c), dy = y - cy(c);
        float r = (float) Math.sqrt(dx * dx + dy * dy);
        float radius = sa(c);
        // つまみは外周の内側に収める
        float k = r > radius ? radius / r : 1f;
        knobX[c] = cx(c) + dx * k;
        knobY[c] = cy(c) + dy * k;
        if (c == RS) {
            // 右スティックは連続値。死区の外を、C++ の死区の外から倒し切りまでに写す
            float dead = radius * RS_DEAD;
            int vx = 0, vy = 0;
            if (r > dead) {
                float mag = Math.min((r - dead) / Math.max(radius - dead, 1f), 1f);
                float v = CURSOR_DEADZONE + mag * (32767 - CURSOR_DEADZONE);
                vx = Math.round(dx / r * v);
                vy = Math.round(dy / r * v);
            }
            SDLActivity.nativePadAxis(DCSSGamepad.AXIS_RIGHTX, vx);
            SDLActivity.nativePadAxis(DCSSGamepad.AXIS_RIGHTY, vy);
            return;
        }
        if (r >= radius * STICK_ENTER) {
            float nx = dx / r, ny = dy / r;
            setStick(nx > OCTANT_SIN ? 1 : nx < -OCTANT_SIN ? -1 : 0,
                     ny > OCTANT_SIN ? 1 : ny < -OCTANT_SIN ? -1 : 0);
        } else if (r <= radius * STICK_LEAVE) {
            setStick(0, 0);
        }
    }

    private void release(int c) {
        if (c == LS) {
            setStick(0, 0);
            pressed[LS] = false;
        } else if (c == RS) {
            SDLActivity.nativePadAxis(DCSSGamepad.AXIS_RIGHTX, 0);
            SDLActivity.nativePadAxis(DCSSGamepad.AXIS_RIGHTY, 0);
            pressed[RS] = false;
        } else {
            press(c, false);
        }
    }

    /** 押しているものを全部離す(隠すとき・画面が回ったとき)。 */
    public void releaseAll() {
        for (int i = 0; i < pointerControl.size(); i++)
            release(pointerControl.valueAt(i));
        pointerControl.clear();
        invalidate();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        releaseAll();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        int index = event.getActionIndex();
        switch (action) {
        case MotionEvent.ACTION_DOWN:
        case MotionEvent.ACTION_POINTER_DOWN: {
            float x = event.getX(index), y = event.getY(index);
            int c = hit(x, y);
            if (c < 0) {
                // 最初の指がボタンの外なら受け取らない(下の画面へ素通り)
                return action != MotionEvent.ACTION_DOWN;
            }
            pointerControl.put(event.getPointerId(index), c);
            if (c == LS || c == RS) {
                pressed[c] = true;
                moveStick(c, x, y);
            } else {
                press(c, true);
            }
            break;
        }
        case MotionEvent.ACTION_MOVE:
            // 一度掴んだボタンは、指を滑らせても替えない。スティックだけ向きを追う
            for (int i = 0; i < event.getPointerCount(); i++) {
                int c = pointerControl.get(event.getPointerId(i), -1);
                if (c == LS || c == RS)
                    moveStick(c, event.getX(i), event.getY(i));
            }
            break;
        case MotionEvent.ACTION_UP:
        case MotionEvent.ACTION_POINTER_UP: {
            int id = event.getPointerId(index);
            int c = pointerControl.get(id, -1);
            if (c >= 0) {
                release(c);
                pointerControl.delete(id);
            }
            break;
        }
        case MotionEvent.ACTION_CANCEL:
            releaseAll();
            break;
        default:
            break;
        }
        invalidate();
        return true;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        float basis = sizeBasis();
        line.setStrokeWidth(Math.max(2f, basis * 0.006f));
        for (int c = 0; c < COUNT; c++) {
            float x = cx(c), y = cy(c), a = sa(c), b = sb(c);
            fill.setColor(pressed[c] ? 0x80FFFFFF : 0x30FFFFFF);
            line.setColor(0x90FFFFFF);
            switch ((int) spec(c)[2]) {
            case RECT:
                rect.set(x - a, y - b, x + a, y + b);
                canvas.drawRoundRect(rect, b, b, fill);
                canvas.drawRoundRect(rect, b, b, line);
                text.setTextSize(b * 1.1f);
                break;
            case STICK:
                fill.setColor(0x20FFFFFF);
                canvas.drawCircle(x, y, a, fill);
                canvas.drawCircle(x, y, a, line);
                float kx = pressed[c] ? knobX[c] : x, ky = pressed[c] ? knobY[c] : y;
                fill.setColor(pressed[c] ? 0x80FFFFFF : 0x40FFFFFF);
                canvas.drawCircle(kx, ky, b, fill);
                canvas.drawCircle(kx, ky, b, line);
                continue;
            default:
                canvas.drawCircle(x, y, a, fill);
                canvas.drawCircle(x, y, a, line);
                text.setTextSize(a * 0.9f);
            }
            text.setColor(0xC0FFFFFF);
            canvas.drawText(LABELS[c], x, y - (text.descent() + text.ascent()) / 2, text);
        }
    }
}
