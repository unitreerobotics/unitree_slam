<#
Windows 版 WiFi 自动路由钩子（对标 Linux NetworkManager dispatcher）
功能：仅连接 192.168.12.x WiFi 时自动添加默认路由，其余情况自动清理
执行一次，永久生效，无任何手动配置
#>

$ErrorActionPreference = "Continue"

# 检查管理员权限（提示用户，但不自动提权，避免窗口闪退）
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "错误：需要管理员权限运行！请右键以管理员身份运行，或使用 RunAddWifiRoute.vbs" -ForegroundColor Red
    pause
    return
}

try {

# ==================== 核心配置（和你Linux脚本逻辑一致）====================
$TARGET_GATEWAY = "192.168.12.1"   # 目标WiFi网关
$TARGET_SUBNET  = "192.168.12."    # 目标网段
$METRIC         = 20               # 优先级（数字越小越优先）
$SCRIPT_PATH    = "C:\Windows\System32\AutoWifiRoute.ps1"
# ========================================================================

# 1. 清理旧配置（避免冲突）
try { Unregister-ScheduledTask -TaskName "AutoWifiRouteHook" -Confirm:$false -ErrorAction Stop } catch {}
Remove-Item -Path $SCRIPT_PATH -Force -ErrorAction SilentlyContinue

# 2. 写入核心钩子脚本（系统目录，永久生效）
# 注意：使用 .StartsWith() 方法代替不支持的 -startsWith 运算符
@"
# 自动WiFi路由钩子（由系统网络事件触发）
`$TARGET_GATEWAY = "$TARGET_GATEWAY"
`$TARGET_SUBNET  = "$TARGET_SUBNET"
`$METRIC         = $METRIC

# 清理所有旧路由（防止残留）
Get-NetRoute -AddressFamily IPv4 -DestinationPrefix 0.0.0.0/0 -ErrorAction SilentlyContinue |
Where-Object { `$_.NextHop -eq `$TARGET_GATEWAY } |
Remove-NetRoute -Confirm:`$false -ErrorAction SilentlyContinue

# 检测已连接的无线网卡
`$wifi = Get-NetAdapter | Where-Object {
    `$_.PhysicalMediaType -match "Native802.11|802.11" -and `$_.Status -eq "Up"
}
if (!`$wifi) { return }

# 检测是否为目标WiFi网段
`$ip = Get-NetIPAddress -InterfaceIndex `$wifi.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
Select-Object -ExpandProperty IPAddress | Where-Object { `$_.StartsWith(`$TARGET_SUBNET) }
if (!`$ip) { return }

# 仅目标WiFi：添加默认路由（强制绑定无线网卡，绝不乱绑）
New-NetRoute -DestinationPrefix 0.0.0.0/0 ``
    -NextHop `$TARGET_GATEWAY ``
    -InterfaceIndex `$wifi.ifIndex ``
    -RouteMetric `$METRIC ``
    -AddressFamily IPv4 -ErrorAction SilentlyContinue | Out-Null
"@ | Out-File $SCRIPT_PATH -Encoding UTF8 -Force

Write-Host "✅ 钩子脚本已写入: $SCRIPT_PATH" -ForegroundColor Green

# 3. 创建触发器
# 注意：New-ScheduledTaskTrigger 不支持 -AtEvent，使用 CIM 实例创建事件触发器
$taskName = "AutoWifiRouteHook"

# 触发条件1：使用 CIM 创建 WiFi 连接事件触发器（EventID=8001）
$CIMTriggerClass = Get-CimClass -ClassName MSFT_TaskEventTrigger -Namespace Root/Microsoft/Windows/TaskScheduler
$eventTrigger = New-CimInstance -CimClass $CIMTriggerClass -ClientOnly
$eventTrigger.Subscription = @"
<QueryList><Query Id="0" Path="Microsoft-Windows-WLAN-AutoConfig/Operational">
<Select Path="Microsoft-Windows-WLAN-AutoConfig/Operational">*[System[EventID=8001]]</Select>
</Query></QueryList>
"@
$eventTrigger.Enabled = $true

# 触发条件2：用户登录/开机
$logonTrigger = New-ScheduledTaskTrigger -AtLogOn

# 合并触发器
$triggers = @($eventTrigger, $logonTrigger)

# 4. 注册系统自动触发任务
$action = New-ScheduledTaskAction -Execute "powershell.exe" `
    -Argument "-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File `"$SCRIPT_PATH`""
$principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $triggers -Principal $principal -Settings $settings -Force

# 5. 立即执行一次（马上生效）
Write-Host "正在立即执行钩子脚本..." -ForegroundColor Yellow
& $SCRIPT_PATH

# 完成提示
Write-Host "`n========================================" -ForegroundColor Green
Write-Host "✅ 安装完成！永久自动生效！（对标Linux钩子）" -ForegroundColor White
Write-Host "🎯 网关：$TARGET_GATEWAY"
Write-Host "📶 规则：仅连接 192.168.12.x WiFi 自动启用路由"
Write-Host "🔌 其他情况：路由自动消失，不影响有线/其他WiFi"
Write-Host "========================================`n" -ForegroundColor Green

# 显示当前路由
Write-Host "当前默认路由：" -ForegroundColor Yellow
Get-NetRoute -AddressFamily IPv4 -DestinationPrefix 0.0.0.0/0 | Format-Table InterfaceAlias,NextHop,RouteMetric -AutoSize

} catch {
    Write-Host "`n❌ 脚本执行出错：" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host $_.ScriptStackTrace -ForegroundColor DarkRed
}

Write-Host "`n按任意键退出..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
