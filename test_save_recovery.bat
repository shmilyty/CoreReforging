@echo off
echo ========================================
echo 测试存档恢复功能
echo ========================================
echo.

echo [测试 1] 删除 saves 文件夹，测试自动创建
if exist saves rmdir /s /q saves
echo saves 文件夹已删除
echo.

echo 启动游戏，应该自动创建 saves 文件夹和存档文件...
echo 1 | game_new.exe
echo.

echo [测试 2] 检查 saves 文件夹是否已创建
if exist saves (
    echo [成功] saves 文件夹已创建
    dir saves
) else (
    echo [失败] saves 文件夹未创建
)
echo.

echo [测试 3] 损坏一个存档文件
echo {invalid json > saves\save_slot_1.json
echo 存档 1 已损坏
echo.

echo 再次启动游戏，应该自动修复损坏的存档...
echo 1 | game_new.exe
echo.

echo [测试 4] 检查存档是否已修复
type saves\save_slot_1.json
echo.

echo ========================================
echo 测试完成
echo ========================================
pause

