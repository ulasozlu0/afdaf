# Build script: kill inj.exe if running, then build Release x64.
$ErrorActionPreference = "Continue"

function Kill-Injector {
    $proc = Get-Process -Name "inj" -ErrorAction SilentlyContinue
    if (-not $proc) {
        Write-Host "[build] inj.exe not running, continuing..."
        return
    }
    $proc | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
    if (Get-Process -Name "inj" -ErrorAction SilentlyContinue) {
        Write-Host "[build] inj.exe is elevated, requesting admin kill (accept the UAC prompt)..."
        try {
            Start-Process powershell -Verb RunAs -Wait -ArgumentList '-NoProfile','-Command','Stop-Process -Name inj -Force -ErrorAction SilentlyContinue'
        }
        catch {
            Write-Host "[build] UAC was declined or failed: $($_.Exception.Message)" -ForegroundColor Yellow
        }
        Start-Sleep -Seconds 1
        if (Get-Process -Name "inj" -ErrorAction SilentlyContinue) {
            Write-Host "[build] WARNING: inj.exe is still running, ctx.dll may stay locked." -ForegroundColor Yellow
        }
        else {
            Write-Host "[build] inj.exe terminated."
        }
    }
    else {
        Write-Host "[build] Terminated inj.exe (PID: $($proc.Id -join ', '))"
    }
}

Kill-Injector

$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path -LiteralPath $msbuild)) {
    Write-Host "[build] MSBuild not found: $msbuild" -ForegroundColor Red
    exit 1
}

& $msbuild project.vcxproj /p:Configuration=Release /p:Platform=x64 /m /v:m /nologo
exit $LASTEXITCODE
