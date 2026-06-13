$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$outputCpp = Join-Path $root "OccResurf\OutputWnd.cpp"
$outputH = Join-Path $root "OccResurf\OutputWnd.h"
$commandCpp = Join-Path $root "OccResurf\CommandEdit.cpp"

$outputText = Get-Content -Raw -Path $outputCpp
$outputHeader = Get-Content -Raw -Path $outputH
$commandText = Get-Content -Raw -Path $commandCpp

function Assert-Contains($text, $pattern, $message) {
    if ($text -notmatch $pattern) {
        throw $message
    }
}

function Assert-NotContains($text, $pattern, $message) {
    if ($text -match $pattern) {
        throw $message
    }
}

Assert-Contains $outputText "CMFCTabCtrl::LOCATION_TOP" "Output tabs should be moved to the top so command logs are easier to find."
Assert-Contains $outputText "m_wndTabs\.SetActiveTab\(0\)" "Command log should be the default active output tab."
Assert-Contains $outputText "m_wndTabs\.AddTab\(&m_wndCommandHistory" "The command log tab should be inserted first."
$commandTabLine = ($outputText -split "`r?`n") | Where-Object { $_ -like "*m_wndTabs.AddTab(&m_wndCommandHistory*" } | Select-Object -First 1
if ($null -eq $commandTabLine -or -not $commandTabLine.Contains("(UINT)0")) {
    throw "The command log tab should use the first tab slot."
}
Assert-Contains $outputText "m_wndCommandLabel\.Create" "A visible command label should be created."
Assert-Contains $outputText "SetCueBanner" "The command edit should show an example placeholder."
Assert-Contains $outputText "m_commandFont\.CreateFontIndirect" "Output pane should own a larger command font."
Assert-Contains $outputText "lf\.lfHeight = MulDiv\(11," "Command UI font should be explicitly raised to 11pt."
Assert-Contains $outputText "SWP_NOZORDER\);" "Command controls should be laid out explicitly."
Assert-Contains $outputHeader "CStatic m_wndCommandLabel" "Output pane header should keep a command label control."
Assert-Contains $outputHeader "CFont m_commandFont" "Output pane header should keep a larger command font."
Assert-Contains $commandText "ON_WM_GETDLGCODE" "Command edit should request dialog keys from MFC."
Assert-Contains $commandText "DLGC_WANTALLKEYS" "Command edit should receive Enter/Tab/arrow keys reliably."
Assert-Contains $commandText "PostMessage\(WM_OCC_COMMAND_ENTERED" "Command submit should post to the pane after the edit text is reset."
Assert-NotContains $outputText "rectClient\.bottom - editHeight" "Command edit should no longer be hidden at the bottom of the tab pane."

Write-Host "command ui regression checks passed"
