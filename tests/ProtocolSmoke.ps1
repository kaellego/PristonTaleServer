param(
    [string]$HostName = '127.0.0.1',
    [int]$Port = 5055,
    [int]$Connections = 100,
    [int]$HoldSeconds = 2,
    [string]$Account = '',
    [string]$PasswordHash = '',
    [int]$Version = 1610,
    [int]$TimeoutMilliseconds = 5000
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

public sealed class ProtocolSmokeResult
{
    public bool Connected;
    public bool Handshake;
    public bool LoginSent;
    public bool LoginAccepted;
    public bool ServerListReceived;
    public bool ClosedCleanly;
    public string Error = "";
}

public static class ProtocolSmokeClient
{
    private const uint LoginUser = 0x48480001;
    private const uint AccountLoginCode = 0x48470023;
    private const uint ServerList = 0x484700C0;

    private static void ReadFull(Stream stream, byte[] buffer)
    {
        var offset = 0;
        while (offset < buffer.Length)
        {
            var read = stream.Read(buffer, offset, buffer.Length - offset);
            if (read == 0) throw new IOException("Conexao encerrada durante a leitura");
            offset += read;
        }
    }

    private static ushort U16(byte[] b, int p) { return (ushort)(b[p] | (b[p + 1] << 8)); }
    private static uint U32(byte[] b, int p) { return (uint)(b[p] | (b[p + 1] << 8) | (b[p + 2] << 16) | (b[p + 3] << 24)); }
    private static void Put32(byte[] b, int p, int value) { Array.Copy(BitConverter.GetBytes(value), 0, b, p, 4); }
    private static void PutString(byte[] b, int p, int length, string value)
    {
        var data = System.Text.Encoding.ASCII.GetBytes(value ?? "");
        Array.Copy(data, 0, b, p, Math.Min(length - 1, data.Length));
    }

    private static byte[] ReadPacket(Stream stream, byte xorKey, byte[] keys, ref byte nextKey, out uint opcode)
    {
        var header = new byte[8]; ReadFull(stream, header);
        for (var i = 0; i < header.Length; i++) header[i] ^= xorKey;
        var length = U16(header, 0); opcode = U32(header, 4);
        if (length < 8 || length > 8192) throw new InvalidDataException("Cabecalho invalido");
        var body = new byte[length - 8]; ReadFull(stream, body);
        for (var i = 0; i < body.Length; i++) body[i] ^= xorKey;
        if (header[3] != 0)
        {
            var keyIndex = header[2]; var obfuscator = (byte)(keys[keyIndex] + keyIndex + 1);
            for (var i = 0; i < body.Length; i++)
            {
                var packetIndex = (ushort)(8 + i);
                obfuscator = (byte)(obfuscator + (packetIndex & 0xff) + (packetIndex >> 8));
                body[i] ^= obfuscator; body[i] ^= (byte)(keyIndex + 2);
            }
            nextKey = (byte)((nextKey + length) & 0xff);
        }
        return body;
    }

    private static void SendPacket(Stream stream, byte xorKey, byte[] keys, ref byte nextKey, uint opcode, byte[] body)
    {
        var packet = new byte[8 + body.Length];
        packet[0] = (byte)packet.Length; packet[1] = (byte)(packet.Length >> 8);
        packet[2] = nextKey; packet[3] = 1;
        Array.Copy(BitConverter.GetBytes(opcode), 0, packet, 4, 4);
        var obfuscator = (byte)(keys[nextKey] + nextKey + 1);
        for (var i = 0; i < body.Length; i++)
        {
            var packetIndex = (ushort)(8 + i);
            obfuscator = (byte)(obfuscator + (packetIndex & 0xff) + (packetIndex >> 8));
            packet[8 + i] = (byte)(body[i] ^ obfuscator ^ (byte)(nextKey + 2));
        }
        for (var i = 0; i < packet.Length; i++) packet[i] ^= xorKey;
        stream.Write(packet, 0, packet.Length); stream.Flush();
        nextKey = (byte)((nextKey + packet.Length) & 0xff);
    }

    public static ProtocolSmokeResult Run(string host, int port, int holdSeconds, string account, string password, int version, int timeout)
    {
        var result = new ProtocolSmokeResult();
        try
        {
            using (var client = new TcpClient())
            {
                client.ReceiveTimeout = timeout; client.SendTimeout = timeout;
                client.Connect(host, port); result.Connected = true;
                using (var stream = client.GetStream())
                {
                    var keyPacket = new byte[266]; ReadFull(stream, keyPacket);
                    if (U16(keyPacket, 0) != 266) throw new InvalidDataException("Pacote de chave invalido");
                    var index = keyPacket[8]; var encrypted = keyPacket[9];
                    var xor = (byte)((version * 519 * 16510 * 14 + 595) % 180 + 20);
                    var keyObfuscator = (byte)(port + index + encrypted);
                    var keys = new byte[256]; for (var i = 0; i < keys.Length; i++) keys[i] = (byte)(keyPacket[10 + i] ^ keyObfuscator);
                    var nextKey = index; result.Handshake = true;
                    if (!String.IsNullOrEmpty(account))
                    {
                        var body = new byte[480]; PutString(body, 12, 32, account); PutString(body, 44, 65, password);
                        PutString(body, 112, 20, "00-00-00-00-00-00"); PutString(body, 132, 32, "ProtocolSmoke");
                        Put32(body, 164, 1); PutString(body, 168, 256, "ProtocolSmoke Video"); PutString(body, 424, 40, "ProtocolSmoke Hardware");
                        Put32(body, 464, 1920); Put32(body, 468, 1080); Put32(body, 472, 1); Put32(body, 476, version);
                        SendPacket(stream, xor, keys, ref nextKey, LoginUser, body); result.LoginSent = true;
                        for (var i = 0; i < 3; i++)
                        {
                            uint opcode; var response = ReadPacket(stream, xor, keys, ref nextKey, out opcode);
                            if (opcode == AccountLoginCode && response.Length >= 12) { result.LoginAccepted = BitConverter.ToInt32(response, 4) == 0; }
                            if (opcode == ServerList) result.ServerListReceived = true;
                        }
                    }
                    Thread.Sleep(Math.Max(0, holdSeconds) * 1000);
                }
            }
            result.ClosedCleanly = true;
        }
        catch (Exception ex) { result.Error = ex.Message; }
        return result;
    }

    public static ProtocolSmokeResult[] RunMany(string host, int port, int count, int holdSeconds, string account, string password, int version, int timeout)
    {
        var tasks = new Task<ProtocolSmokeResult>[count];
        for (var i = 0; i < count; i++)
        {
            tasks[i] = Task.Run(() => Run(host, port, holdSeconds, account, password, version, timeout));
        }
        Task.WaitAll(tasks);
        var results = new ProtocolSmokeResult[count];
        for (var i = 0; i < count; i++) results[i] = tasks[i].Result;
        return results;
    }
}
'@

$results = @([ProtocolSmokeClient]::RunMany($HostName, $Port, $Connections, $HoldSeconds, $Account, $PasswordHash, $Version, $TimeoutMilliseconds))
$connected = @($results | Where-Object Connected).Count
$handshakes = @($results | Where-Object Handshake).Count
$closed = @($results | Where-Object ClosedCleanly).Count
$logins = @($results | Where-Object LoginSent).Count
$accepted = @($results | Where-Object LoginAccepted).Count
$serverLists = @($results | Where-Object ServerListReceived).Count

Write-Host "Conexoes: $connected/$Connections | Handshake: $handshakes/$Connections | Encerramento limpo: $closed/$Connections"
if ($logins -gt 0) { Write-Host "Login enviado: $logins | Login aceito: $accepted | Lista de servidores: $serverLists" }
$errors = @($results | Where-Object { -not $_.ClosedCleanly -and $_.Error } | Select-Object -ExpandProperty Error -Unique)
if ($errors.Count -gt 0) { Write-Host "Erros:"; $errors | ForEach-Object { Write-Host " - $_" } }
if ($connected -ne $Connections -or $handshakes -ne $Connections -or $closed -ne $Connections) { exit 1 }
if ($logins -gt 0 -and ($accepted -ne $logins -or $serverLists -ne $logins)) { exit 2 }
Write-Host 'PASS: teste de protocolo concluido.'
