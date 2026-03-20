using System;
using System.Collections;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.UI;
using Unity.WebRTC;

/*
 * WebRtcReceiver.cs — M8: Unity WebRTC Alıcı (com.unity.webrtc)
 *
 * RPi'den gelen WebRTC stream'ini alır:
 *   - Video: VideoStreamTrack → RawImage texture
 *   - Motor: RTCDataChannel → JSON komut gönder
 *
 * Gereksinim: Window → Package Manager → com.unity.webrtc
 *
 * Kurulum:
 *   1. VideoManager'a bu scripti ekle
 *   2. Inspector'da signalUrl ve displayImage ayarla
 *   3. RPi'de webrtc_signal.py + webrtc_peer.py çalıştır
 *   4. Play
 */
public class WebRtcReceiver : MonoBehaviour
{
    [Header("Signaling")]
    public string signalUrl = "ws://192.168.1.126:8766/ws?role=unity";

    [Header("Görüntü")]
    public RawImage displayImage;

    private RTCPeerConnection _pc;
    private RTCDataChannel _motorChannel;
    private ClientWebSocket _ws;
    private CancellationTokenSource _cts;

    void Awake()
    {
        Debug.Log("[WEBRTC] Awake() çağrıldı ✅");
    }

    void Start()
    {
        Debug.Log("[WEBRTC] Start() çağrıldı ✅");
        _cts = new CancellationTokenSource();
        // com.unity.webrtc 3.0 için zorunlu update döngüsü
        StartCoroutine(WebRTCUpdateLoop());
        StartCoroutine(ConnectSignaling());
    }

    IEnumerator WebRTCUpdateLoop()
    {
        while (true)
            yield return WebRTC.Update();
    }

    IEnumerator ConnectSignaling()
    {
        Debug.Log($"[WEBRTC] Signaling bağlanıyor: {signalUrl}");
        _ws = new ClientWebSocket();
        var connectTask = _ws.ConnectAsync(new Uri(signalUrl), _cts.Token);
        yield return new WaitUntil(() => connectTask.IsCompleted);

        if (connectTask.IsFaulted)
        {
            Debug.LogError($"[WEBRTC] Bağlantı hatası: {connectTask.Exception?.GetBaseException().Message}");
            yield break;
        }
        if (_ws.State != System.Net.WebSockets.WebSocketState.Open)
        {
            Debug.LogError($"[WEBRTC] Signaling bağlantısı başarısız: {signalUrl}");
            yield break;
        }
        Debug.Log("[WEBRTC] Signaling'e bağlandı ✅");

        // Mesaj okuma döngüsü
        StartCoroutine(ReceiveLoop());
    }

    IEnumerator ReceiveLoop()
    {
        var buffer = new byte[16384];
        while (_ws != null && _ws.State == System.Net.WebSockets.WebSocketState.Open)
        {
            var receiveTask = _ws.ReceiveAsync(new ArraySegment<byte>(buffer), _cts.Token);
            yield return new WaitUntil(() => receiveTask.IsCompleted);

            if (receiveTask.Result.MessageType == WebSocketMessageType.Text)
            {
                string json = Encoding.UTF8.GetString(buffer, 0, receiveTask.Result.Count);
                HandleSignalMessage(json);
            }
        }
    }

    void HandleSignalMessage(string json)
    {
        var msg = JsonUtility.FromJson<SignalMsg>(json);
        Debug.Log($"[WEBRTC] Mesaj alındı: {msg.type}");

        if (msg.type == "offer")
            StartCoroutine(HandleOffer(msg.sdp));
    }

    private VideoStreamTrack _videoTrack;

    IEnumerator HandleOffer(string sdp)
    {
        var config = new RTCConfiguration
        {
            iceServers = new[] { new RTCIceServer { urls = new[] { "stun:stun.l.google.com:19302" } } }
        };
        _pc = new RTCPeerConnection(ref config);

        // Video almak için transceiver ekle (RecvOnly)
        _pc.AddTransceiver(TrackKind.Video, new RTCRtpTransceiverInit
        {
            direction = RTCRtpTransceiverDirection.RecvOnly
        });

        // Video track geldiğinde sakla
        _pc.OnTrack = (e) =>
        {
            if (e.Track is VideoStreamTrack videoTrack)
            {
                _videoTrack = videoTrack;
                _videoTrack.enabled = true;
                Debug.Log("[WEBRTC] Video track alındı ✅");
            }
        };

        // DataChannel callback
        _pc.OnDataChannel = (channel) =>
        {
            _motorChannel = channel;
            Debug.Log($"[WEBRTC] DataChannel: {channel.Label}");
        };

        // ICE candidate gönder
        _pc.OnIceCandidate = (candidate) => SendSignal(new CandidateMsg
        {
            type = "candidate",
            candidate = candidate.Candidate,
            sdpMid = candidate.SdpMid,
            sdpMLineIndex = candidate.SdpMLineIndex ?? 0
        });

        // Remote description (offer)
        var offerDesc = new RTCSessionDescription { type = RTCSdpType.Offer, sdp = sdp };
        var setRemote = _pc.SetRemoteDescription(ref offerDesc);
        yield return setRemote;

        // Answer oluştur
        var answerOp = _pc.CreateAnswer();
        yield return answerOp;

        var answer = answerOp.Desc;
        var setLocal = _pc.SetLocalDescription(ref answer);
        yield return setLocal;

        SendSignal(new SignalMsg { type = "answer", sdp = answer.sdp });
        Debug.Log("[WEBRTC] Answer gönderildi ✅");
    }

    void Update()
    {
        // Video texture'ı her frame güncelle
        if (_videoTrack != null && displayImage != null)
        {
            var tex = _videoTrack.Texture;
            if (tex != null && displayImage.texture != tex)
            {
                displayImage.texture = tex;
                Debug.Log("[WEBRTC] Video texture atandı ✅");
            }
        }
    }

    void SendSignal<T>(T msg)
    {
        if (_ws == null || _ws.State != System.Net.WebSockets.WebSocketState.Open) return;
        string json = JsonUtility.ToJson(msg);
        byte[] bytes = Encoding.UTF8.GetBytes(json);
        _ws.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, _cts.Token);
    }

    /// <summary>Motor komutunu DataChannel üzerinden gönder.</summary>
    public void SendMotorCommand(string direction)
    {
        if (_motorChannel != null && _motorChannel.ReadyState == RTCDataChannelState.Open)
        {
            _motorChannel.Send($"{{\"dir\":\"{direction}\"}}");
        }
    }

    void OnDestroy()
    {
        _cts?.Cancel();
        _pc?.Close();
        _ws?.CloseAsync(WebSocketCloseStatus.NormalClosure, "Uygulama kapandı", default);
    }

    [Serializable] class SignalMsg { public string type; public string sdp; }
    [Serializable] class CandidateMsg
    {
        public string type;
        public string candidate;
        public string sdpMid;
        public int sdpMLineIndex;
    }
}
