param(
    [string]$HostName = '127.0.0.1',
    [int]$Port = 5055,
    [int]$Connections = 100
)

$clients = [System.Collections.Generic.List[System.Net.Sockets.TcpClient]]::new()
try {
    1..$Connections | ForEach-Object {
        $client = [System.Net.Sockets.TcpClient]::new()
        $client.Connect($HostName, $Port)
        $clients.Add($client)
    }
    $ready = ($clients | Where-Object { $_.Connected }).Count
    if ($ready -ne $Connections) { throw "Only $ready of $Connections connections were established." }
    Write-Host "PASS: $ready TCP connections established."
}
finally {
    $clients | ForEach-Object { $_.Dispose() }
}
