# 1. 编译 C++ 文件
Write-Host "正在编译..." -ForegroundColor Cyan
g++ main.cpp GameCore.cpp -o game.exe

# 2. 检查编译结果 ($LASTEXITCODE 为 0 表示成功)
if ($LASTEXITCODE -eq 0) {
    Write-Host "编译成功，正在启动游戏..." -ForegroundColor Green
    
    # 分隔线，为了看清游戏输出
    Write-Host "----------------------------------------" 
    
    # 3. 运行游戏
    .\game.exe
    
    # 4. 游戏结束后暂停
    Write-Host "`n游戏已结束。" -ForegroundColor Yellow
    Write-Host "将在 3 秒后关闭窗口..."
    Start-Sleep -Seconds 3
}
else {
    # 编译失败时的处理
    Write-Host "编译失败，请检查代码错误。" -ForegroundColor Red
    # 发生错误时暂停较长时间以便查看报错
    Read-Host "按回车键退出..."
}