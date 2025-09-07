# hw0 report

|Field|Value|
|-:|:-|
|Name|林彥宇|
|ID|112550098|

## How much time did you spend on this project

About 3-4 hours

## Project overview

以前 DCL 已經裝過 Vivado 了
但我裝的是2024年版本 希望 2025 沒有差很多

今年蔡淳仁老師有對他的系統新增了 float 運算
聽說在 Lab5 會加速 6~7 倍
還好我都還沒寫去年的作業

在安裝 riscv-gnu-toolchain 花了一陣子 可能需要 1hr

因為我是用 wsl 寫的，所以用 GPT 寫了一個腳本讓 make 完之後的 .elf 可以 export 到桌面

在 Vivado Simulation 完後，可以直接在 tcl console 打
run all
這樣才會跑完所有程式的波形圖
如果要看子元件的波形圖 要先 add to waveform 再重 simulate 一次

最後更改了 elib 中的 string.c 裡面 `strcpy()` 跟 `strcmp()` 來改進 DMIPS/Mhz
執行結果:
```
It tooks     0.01 seconds.
Microseconds for one run through Dhrystone: 14.806000 
Dhrystones per Second:                      67540.2 
VAX MIPS:                                     38.4 
DMIPS/Mhz:                                     0.77
```

```
It tooks     0.01 seconds.
Microseconds for one run through Dhrystone: 11.826000 
Dhrystones per Second:                      84559.4 
VAX MIPS:                                     48.1 
DMIPS/Mhz:                                     0.96
```

## What is the hardest you think in this project

熟悉整個 HW-SW 架構
但看波形圖有點回想到寫祭祖的感覺
還蠻好玩的

## Code

export-elf
```shell
#!/usr/bin/env bash
# export-elf — copy the newest .elf from a build dir to a Windows folder
# Usage:
#   export-elf [--from DIR] [DEST]
#   - If DEST is omitted, it uses your preset Windows folder.
#   - DEST can be Windows path (e.g., C:\Users\Apollo\...) or WSL path (/mnt/c/...).
#   - --from DIR sets where to search for .elf (default: current directory).

set -euo pipefail

search_dir="$PWD"
dest_arg=""

# --- Parse args ---
while [[ $# -gt 0 ]]; do
  case "$1" in
    --from)
      [[ $# -ge 2 ]] || { echo "ERROR: --from needs a directory"; exit 1; }
      search_dir="$2"; shift 2;;
    -*)
      echo "Unknown option: $1"; exit 1;;
    *)
      dest_arg="$1"; shift;;
  esac
done

# --- Find the newest .elf ---
latest_elf="$(find "$search_dir" -type f -name '*.elf' -print0 | xargs -0r ls -t 2>/dev/null | head -n1 || true)"
if [[ -z "${latest_elf}" ]]; then
  echo "ERROR: No .elf found under: $search_dir"
  exit 1
fi

# --- Convert Windows path to WSL path ---
to_wsl_path() {
  local p="$1"
  if [[ "$p" =~ ^[A-Za-z]:\\ ]]; then
    wslpath -u "$p"
  else
    echo "$p"
  fi
}

# --- Decide destination folder ---
if [[ -z "$dest_arg" ]]; then
  # 預設路徑：C:\Users\Apollo\Desktop\NYCU\semester5\mpd\elf_export
  dest_dir="$(to_wsl_path 'C:\Users\Apollo\Desktop\NYCU\semester5\mpd\elf_export')"
else
  dest_dir="$(to_wsl_path "$dest_arg")"
fi

mkdir -p "$dest_dir"

# --- Copy ---
cp -f "$latest_elf" "$dest_dir/"

dst_file="$dest_dir/$(basename "$latest_elf")"
dst_win="$(wslpath -w "$dst_file")"

echo "Source : $latest_elf"
echo "Copied → $dst_file"
echo "Windows: $dst_win"

```

