@echo off
setlocal enabledelayedexpansion

chcp 65001 > nul
echo ===================================================
echo  PIC16F1455 USB HID - Quick Git Commit and Push
echo ===================================================
echo.

:: Gitステータス確認
git status --short
echo.

:: コミットメッセージ入力
set /p COMMIT_MSG="Input commit message (Press Enter to Cancel): "
if "%COMMIT_MSG%"=="" (
    echo [CANCEL] コミットメッセージが空のため処理を中止しました。
    pause
    exit /b 0
)

echo.
echo [1/3] Adding files...
git add .

echo [2/3] Committing...
git commit -m "%COMMIT_MSG%"
if errorlevel 1 (
    echo [INFO] コミットする変更がありませんでした。
    pause
    exit /b 0
)

echo [3/3] Pushing to remote...
:: 現在のチェックアウト中ブランチへプッシュ
git push origin HEAD

if errorlevel 1 (
    echo.
    echo [ERROR] プッシュに失敗しました。リモート設定またはネットワークを確認してください。
) else (
    echo.
    echo ===================================================
    echo  Push completed successfully!
    echo ===================================================
)

pause