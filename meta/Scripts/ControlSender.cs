using System;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

/*
 * ControlSender.cs — M10: Unity → Cloud Relay → RPi
 *
 * Cloud relay'e bağlanır, motor + servo komutları gönderir,
 * video frame'leri (binary JPEG) alır.
 */
public class ControlSender : MonoBehaviour
{
    [Header("Cloud Relay")]
    [Tooltip("Relay sunucu adresi")]
    public string relayUrl = "wss://vr-relay-ricg.onrender.com/ws";
    [Tooltip("Oda kodu (RPi'den alınan 6 haneli kod)")]
    public string roomCode = "";

    [Header("Gönderim")]
    public int sendIntervalMs = 50;

    [Header("Güvenlik")]
    public float deadzone  = 0.15f;
    public float maxSpeed  = 80f;

    // Video frame callback — HttpVideoReceiver bu event'i dinler
    public static event Action<byte[]> OnVideoFrame;

    // Bağlantı durumu
    public static bool IsConnected { get; private set; }
    public static string Status { get; private set; } = "Bağlantı yok";

    private InputReader _input;
    private ClientWebSocket _ws;
    private CancellationTokenSource _cts;
    private bool _destroyed = false;

    void Start()
    {
        _input = GetComponent<InputReader>();
        if (_input == null)
        {
            Debug.LogError("[SENDER] InputReader bulunamadı!");
            enabled = false;
            return;
        }
    }

    // RoomConnectUI.cs tarafından çağrılır
    public void Connect(string code)
    {
        roomCode = code;
        _ = ConnectLoop();
    }

    async Task ConnectLoop()
    {
        while (!_destroyed)
        {
            _cts = new CancellationTokenSource();
            _ws  = new ClientWebSocket();

            string uri = $"{relayUrl}?role=quest&room={roomCode}";

            try
            {
                Status = "Bağlanıyor...";
                Debug.Log($"[SENDER] Bağlanıyor: {uri}");
                await _ws.ConnectAsync(new Uri(uri), _cts.Token);

                // İlk mesajı al (connected veya error)
                var buf = new byte[4096];
                var result = await _ws.ReceiveAsync(new ArraySegment<byte>(buf), _cts.Token);
                string msg = Encoding.UTF8.GetString(buf, 0, result.Count);
                var data = JsonUtility.FromJson<RelayMessage>(msg);

                if (data.type == "error")
                {
                    Status = $"Hata: {data.msg}";
                    Debug.LogError($"[SENDER] {data.msg}");
                    break;  // Yanlış kod — tekrar deneme
                }

                if (data.type == "connected")
                {
                    IsConnected = true;
                    Status = "Bağlandı ✓";
                    Debug.Log("[SENDER] RPi ile eşleşildi ✓");
                    await SessionLoop();
                }
            }
            catch (Exception e)
            {
                if (!_destroyed)
                    Debug.LogWarning($"[SENDER] Hata: {e.Message}");
            }
            finally
            {
                IsConnected = false;
                Status = "Bağlantı kesildi";
                _ws?.Dispose();
            }

            if (_destroyed) break;
            await Task.Delay(3000);
        }
    }

    async Task SessionLoop()
    {
        // Paralel: komut gönder + video al
        var sendTask = SendCommands();
        var recvTask = ReceiveFrames();

        await Task.WhenAny(sendTask, recvTask);
        IsConnected = false;
    }

    async Task SendCommands()
    {
        var ic = System.Globalization.CultureInfo.InvariantCulture;
        while (IsConnected && _ws.State == WebSocketState.Open && !_destroyed)
        {
            string dir = HesaplaYon(_input.JoystickX, _input.JoystickY);
            float  spd = Mathf.Clamp(new Vector2(_input.JoystickX, _input.JoystickY).magnitude * 100f, 0f, maxSpeed);

            string json = "{\"dir\":\"" + dir + "\",\"spd\":" +
                          spd.ToString("F0", ic) +
                          ",\"yaw\":" + _input.HeadYaw.ToString("F1", ic) +
                          ",\"pitch\":" + _input.HeadPitch.ToString("F1", ic) + "}";

            var bytes = Encoding.UTF8.GetBytes(json);
            try
            {
                await _ws.SendAsync(new ArraySegment<byte>(bytes),
                    WebSocketMessageType.Text, true, _cts.Token);
            }
            catch { break; }

            await Task.Delay(sendIntervalMs, _cts.Token);
        }
    }

    async Task ReceiveFrames()
    {
        var buffer = new byte[500000]; // 500KB max JPEG
        while (IsConnected && _ws.State == WebSocketState.Open && !_destroyed)
        {
            try
            {
                var result = await _ws.ReceiveAsync(new ArraySegment<byte>(buffer), _cts.Token);

                if (result.MessageType == WebSocketMessageType.Binary)
                {
                    // Video frame (JPEG)
                    var jpeg = new byte[result.Count];
                    Array.Copy(buffer, jpeg, result.Count);
                    OnVideoFrame?.Invoke(jpeg);
                }
                else if (result.MessageType == WebSocketMessageType.Text)
                {
                    string msg = Encoding.UTF8.GetString(buffer, 0, result.Count);
                    var data = JsonUtility.FromJson<RelayMessage>(msg);
                    if (data.type == "peer_disconnected")
                    {
                        Debug.LogWarning("[SENDER] RPi bağlantısı koptu");
                        Status = "RPi bağlantısı koptu";
                        break;
                    }
                }
            }
            catch { break; }
        }
    }

    string HesaplaYon(float x, float y)
    {
        if (Mathf.Abs(x) < deadzone && Mathf.Abs(y) < deadzone) return "stop";
        if (Mathf.Abs(y) >= Mathf.Abs(x)) return y > 0 ? "fwd" : "bwd";
        return x > 0 ? "right" : "left";
    }

    void OnDestroy()
    {
        _destroyed = true;
        _cts?.Cancel();
        _ws?.Dispose();
    }

    [Serializable]
    private class RelayMessage
    {
        public string type;
        public string msg;
        public string code;
    }
}
