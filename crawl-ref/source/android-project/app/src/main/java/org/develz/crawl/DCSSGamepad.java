package org.develz.crawl;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import org.libsdl.app.SDLActivity;

/**
 * 日本語版: 物理のゲームパッドを、Windows 版と同じゲームパッドの処理(gamepad.cc)へ流す。
 *
 * 同梱の SDL 2.0.7 は Android のパッドをほとんど認めず(対応表が無い)、ボタンも
 * SDLActivity.dispatchKeyEvent が先に画面のキーボードへ回してしまう。そこで、パッドから
 * 来たキーと軸をここで取り分け、JNI(nativePadButton / nativePadAxis)で C++ に渡す。
 * C++ 側は SDL の偽のパッドの出来事として積むので、検査用の台本(CRAWL_PAD_TEST)と同じ道を通る。
 * 設計は ja-tools/GAMEPAD_DESIGN.md §9。
 */
public class DCSSGamepad {

    // SDL_GameControllerButton の値(SDL_gamecontroller.h)
    public static final int BUTTON_A = 0;
    public static final int BUTTON_B = 1;
    public static final int BUTTON_X = 2;
    public static final int BUTTON_Y = 3;
    public static final int BUTTON_BACK = 4;
    public static final int BUTTON_START = 6;
    public static final int BUTTON_LEFTSTICK = 7;
    public static final int BUTTON_RIGHTSTICK = 8;
    public static final int BUTTON_LEFTSHOULDER = 9;
    public static final int BUTTON_RIGHTSHOULDER = 10;
    public static final int BUTTON_DPAD_UP = 11;
    public static final int BUTTON_DPAD_DOWN = 12;
    public static final int BUTTON_DPAD_LEFT = 13;
    public static final int BUTTON_DPAD_RIGHT = 14;

    // SDL_GameControllerAxis の値
    public static final int AXIS_LEFTX = 0;
    public static final int AXIS_LEFTY = 1;
    public static final int AXIS_TRIGGERLEFT = 4;
    public static final int AXIS_TRIGGERRIGHT = 5;

    // 最後に送った軸の値(変わったときだけ送る)と、HAT の十字の押し下げ
    private static final int[] lastAxis = { 0, 0, 0, 0, 0, 0 };
    private static int lastHatX = 0, lastHatY = 0;

    /** 機器がゲームパッド(またはジョイスティック)か。テレビのリモコン(十字だけ)は含めない。 */
    public static boolean isGamepad(InputDevice device) {
        if (device == null || device.isVirtual())
            return false;
        int sources = device.getSources();
        return (sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
            || (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    /** いまゲームパッドが繋がっているか。 */
    public static boolean isConnected() {
        for (int id : InputDevice.getDeviceIds()) {
            if (isGamepad(InputDevice.getDevice(id)))
                return true;
        }
        return false;
    }

    private static int buttonFor(int keyCode) {
        switch (keyCode) {
        case KeyEvent.KEYCODE_BUTTON_A: return BUTTON_A;
        case KeyEvent.KEYCODE_BUTTON_B: return BUTTON_B;
        case KeyEvent.KEYCODE_BUTTON_X: return BUTTON_X;
        case KeyEvent.KEYCODE_BUTTON_Y: return BUTTON_Y;
        case KeyEvent.KEYCODE_BUTTON_L1: return BUTTON_LEFTSHOULDER;
        case KeyEvent.KEYCODE_BUTTON_R1: return BUTTON_RIGHTSHOULDER;
        case KeyEvent.KEYCODE_BUTTON_SELECT: return BUTTON_BACK;
        case KeyEvent.KEYCODE_BUTTON_START: return BUTTON_START;
        case KeyEvent.KEYCODE_BUTTON_THUMBL: return BUTTON_LEFTSTICK;
        case KeyEvent.KEYCODE_BUTTON_THUMBR: return BUTTON_RIGHTSTICK;
        case KeyEvent.KEYCODE_DPAD_UP: return BUTTON_DPAD_UP;
        case KeyEvent.KEYCODE_DPAD_DOWN: return BUTTON_DPAD_DOWN;
        case KeyEvent.KEYCODE_DPAD_LEFT: return BUTTON_DPAD_LEFT;
        case KeyEvent.KEYCODE_DPAD_RIGHT: return BUTTON_DPAD_RIGHT;
        default: return -1;
        }
    }

    /**
     * パッドから来たキーなら C++ へ渡して true。ほかのキーは false(今までどおりの処理へ)。
     * SDLActivity.dispatchKeyEvent の先頭から呼ぶ。
     */
    public static boolean handleKey(KeyEvent event) {
        if (!isGamepad(event.getDevice()))
            return false;
        int keyCode = event.getKeyCode();
        int action = event.getAction();
        if (action != KeyEvent.ACTION_DOWN && action != KeyEvent.ACTION_UP)
            return false;
        boolean down = action == KeyEvent.ACTION_DOWN;

        // ボタンのトリガ(L2/R2)は、軸の無いパッドではキーで来る
        if (keyCode == KeyEvent.KEYCODE_BUTTON_L2 || keyCode == KeyEvent.KEYCODE_BUTTON_R2) {
            if (!down || event.getRepeatCount() == 0) {
                sendAxis(keyCode == KeyEvent.KEYCODE_BUTTON_L2 ? AXIS_TRIGGERLEFT
                                                               : AXIS_TRIGGERRIGHT,
                         down ? 32767 : 0, true);
            }
            return true;
        }

        int button = buttonFor(keyCode);
        if (button < 0)
            return false; // BACK などは今までどおり(BACK は Esc)
        // 押し続けたときの繰り返しは C++ の状態機械がやるので、2 回目以降は捨てる
        if (!down || event.getRepeatCount() == 0)
            SDLActivity.nativePadButton(button, down);
        SDLActivity.onGamepadUsed();
        return true;
    }

    private static void sendAxis(int axis, int value, boolean force) {
        if (!force && lastAxis[axis] == value)
            return;
        lastAxis[axis] = value;
        SDLActivity.nativePadAxis(axis, value);
    }

    private static int scale(float v) {
        return Math.max(-32768, Math.min(32767, Math.round(v * 32767f)));
    }

    private static void sendHat(int value, int last, int negButton, int posButton) {
        if (value == last)
            return;
        if (last < 0) SDLActivity.nativePadButton(negButton, false);
        if (last > 0) SDLActivity.nativePadButton(posButton, false);
        if (value < 0) SDLActivity.nativePadButton(negButton, true);
        if (value > 0) SDLActivity.nativePadButton(posButton, true);
    }

    /**
     * パッドの軸(左スティック・十字の HAT・トリガ)なら C++ へ渡して true。
     * 右スティックは送らない(Android ではマウスのカーソルを動かせない。GAMEPAD_DESIGN.md §2.3)。
     */
    public static boolean handleMotion(MotionEvent event) {
        if (event.getActionMasked() != MotionEvent.ACTION_MOVE
            || !isGamepad(event.getDevice())
            || (event.getSource() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK) {
            return false;
        }
        sendAxis(AXIS_LEFTX, scale(event.getAxisValue(MotionEvent.AXIS_X)), false);
        sendAxis(AXIS_LEFTY, scale(event.getAxisValue(MotionEvent.AXIS_Y)), false);
        float lt = Math.max(event.getAxisValue(MotionEvent.AXIS_LTRIGGER),
                            event.getAxisValue(MotionEvent.AXIS_BRAKE));
        float rt = Math.max(event.getAxisValue(MotionEvent.AXIS_RTRIGGER),
                            event.getAxisValue(MotionEvent.AXIS_GAS));
        sendAxis(AXIS_TRIGGERLEFT, Math.max(0, scale(lt)), false);
        sendAxis(AXIS_TRIGGERRIGHT, Math.max(0, scale(rt)), false);

        // 十字を HAT(軸)で送ってくるパッドが多い
        int hatX = Math.round(event.getAxisValue(MotionEvent.AXIS_HAT_X));
        int hatY = Math.round(event.getAxisValue(MotionEvent.AXIS_HAT_Y));
        sendHat(hatX, lastHatX, BUTTON_DPAD_LEFT, BUTTON_DPAD_RIGHT);
        sendHat(hatY, lastHatY, BUTTON_DPAD_UP, BUTTON_DPAD_DOWN);
        lastHatX = hatX;
        lastHatY = hatY;
        SDLActivity.onGamepadUsed();
        return true;
    }
}
