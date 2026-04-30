#!powershell.exe -File
#
# SPDX-FileCopyrightText: (c) 2026 Robert Di Pardo
# SPDX-License-Identifier: MPL-2.0
#
param (
    [string]$Path = "${PSScriptRoot}"
)
$checksums=@()
$hashfile="${Path}\HTMLTag.md5sums"
try {
    pushd "$Path"
    foreach($cfg in @(Get-ChildItem '.' -Filter *.ini))
    {
        $md5=$(Get-fileHash -A MD5 $cfg.FullName).Hash
        $checksums += "$md5 $(Split-Path $cfg -leaf)"
    }
    $checksums += ""
    [System.IO.File]::WriteAllText("$hashfile", ($checksums -join "`n"))
} catch {
    $_.InvocationInfo.PositionMessage; $_.Exception.Message
} finally {
    popd
}
