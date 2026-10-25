# 実行時の未訳探し: 闘技場を画面なしで各所で回し、辞書に無かったキー
# (CRAWL_JA_MISSING)とメッセージ(arena.result)を集める。6 本ずつ並べて回す。
# 使い方(リポジトリのルートで):
#   powershell -File ja-tools\untrans\arena_sweep.ps1 -Out <出力フォルダ> [-Places D:2,Lair:3] [-Trials 12]
# まとめは python -I ja-tools/untrans/arena_summary.py <出力フォルダ>
param(
    [Parameter(Mandatory = $true)][string]$Out,
    [string[]]$Places = @("D:2","D:8","D:14","Lair:3","Orc:2","Swamp:3","Shoals:3","Snake:3",
        "Spider:3","Slime:4","Elf:2","Vaults:3","Crypt:3","Tomb:2","Depths:3","Zot:4","Abyss:3",
        "Pan","Dis:5","Geh:5","Coc:5","Tar:5","Hell","Sewer","Ossuary","Bailey","IceCv","Volcano",
        "Wizlab","Desolation","Gauntlet","Bazaar","Trove","Temple","Zig:13","Zig:27"),
    [int]$Trials = 12,
    [int]$Parallel = 6,
    [int]$TimeoutSec = 300
)
# powershell -File ではカンマ区切りが 1 つの文字列で届く
$Places = @($Places | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$exe = Join-Path $root "crawl-ref\source\crawl.exe"
New-Item -ItemType Directory -Force $Out | Out-Null
$Out = (Resolve-Path $Out).Path
$rc = Join-Path $Out "ja.rc"
"language = ja`narena_delay = 0`narena_dump_msgs = true`narena_list_eq = true" |
    Set-Content -Encoding ascii $rc

function Watch($running) {
    foreach ($r in $running) {
        if (-not $r.p.HasExited -and ((Get-Date) - $r.t).TotalSeconds -gt $TimeoutSec) {
            Stop-Process -Id $r.p.Id -Force
            "$($r.pl) timeout"
        }
    }
}

$running = @()
foreach ($pl in $Places) {
    while (@($running | Where-Object { -not $_.p.HasExited }).Count -ge $Parallel) {
        Watch $running
        Start-Sleep 2
    }
    $w = Join-Path $Out ("w-" + ($pl -replace ':', '_'))
    New-Item -ItemType Directory -Force $w | Out-Null
    $env:CRAWL_JA_MISSING = Join-Path $w "missing.txt"
    $teams = "4 random v 4 random t:$Trials cycle_random random_uniques miscasts summon_throttle:5 arena_place:$pl"
    $p = Start-Process -FilePath $exe -WorkingDirectory $w -PassThru -WindowStyle Hidden `
        -ArgumentList "-headless", "-rc", $rc, "-arena", "`"$teams`""
    $running += [pscustomobject]@{ p = $p; t = Get-Date; pl = $pl }
}
while (@($running | Where-Object { -not $_.p.HasExited }).Count -gt 0) {
    Watch $running
    Start-Sleep 2
}
"done"
