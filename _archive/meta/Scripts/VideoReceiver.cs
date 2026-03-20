using System;
using System.Net;
using System.Net.Sockets;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

/*
 * VideoReceiver.cs — M5: RPi UDP RTP MJPEG Stream Alıcı
 *
 * RPi'den gelen RTP/JPEG paketlerini alır, texture'e dönüştürür
 * ve sahnede bir RawImage üzerinde gösterir.
 *
 * Kurulum:
 *   1. Sahneye Canvas → RawImage ekle
 *   2. Bu scripti bir GameObject'e ekle
 *   3. Inspector'da RawImage'ı sürükle
 *
 * Not: Unity'nin WebCamTexture API'si yerine raw UDP kullanılıyor
 * çünkü stream RPi'den geliyor (lokal kamera değil).
 */
public class VideoReceiver : MonoBehaviour
{
    [Header("Ağ Ayarları")]
    public int listenPort = 5600;

    [Header("Görüntü")]
    public RawImage displayImage;

    private UdpClient _udp;
    private Texture2D _tex;
    private byte[] _frameBuffer;
    private bool _newFrame = false;
    private readonly object _lock = new object();

    // RTP header: 12 byte sabit + payload
    private const int RTP_HEADER_SIZE = 12;
    private int _packetCount = 0;

    void Start()
    {
        _tex = new Texture2D(640, 480, TextureFormat.RGB24, false);
        if (displayImage != null)
            displayImage.texture = _tex;

        _udp = new UdpClient(listenPort);
        _udp.BeginReceive(OnReceive, null);
        Debug.Log($"[VIDEO] UDP dinleniyor: port {listenPort}");
    }

    void OnReceive(IAsyncResult ar)
    {
        try
        {
            var ep = new IPEndPoint(IPAddress.Any, 0);
            byte[] data = _udp.EndReceive(ar, ref ep);

            // RTP header'ı atla, payload = JPEG
            if (data.Length > RTP_HEADER_SIZE)
            {
                int payloadLen = data.Length - RTP_HEADER_SIZE;
                byte[] jpeg = new byte[payloadLen];
                Array.Copy(data, RTP_HEADER_SIZE, jpeg, 0, payloadLen);

                lock (_lock)
                {
                    _frameBuffer = jpeg;
                    _newFrame = true;
                    _packetCount++;
                }
            }
        }
        catch (Exception e)
        {
            if (!(e is ObjectDisposedException))
                Debug.LogWarning($"[VIDEO] Receive hatası: {e.Message}");
        }
        finally
        {
            // UdpClient kapatılmadıysa dinlemeye devam et
            try { _udp?.BeginReceive(OnReceive, null); }
            catch { /* kapatılmış, normal */ }
        }
    }

    void Update()
    {
        // Her 3 saniyede paket sayısını logla
        if (Time.frameCount % 90 == 0)
            Debug.Log($"[VIDEO] Paket: {_packetCount}");
        if (_newFrame)
        {
            byte[] jpeg;
            lock (_lock)
            {
                jpeg = _frameBuffer;
                _newFrame = false;
            }

            try
            {
                _tex.LoadImage(jpeg);
                _tex.Apply();
            }
            catch (Exception e)
            {
                Debug.LogWarning($"[VIDEO] Texture hatası: {e.Message}");
            }
        }
    }

    void OnDestroy()
    {
        _udp?.Close();
        Debug.Log("[VIDEO] UDP kapatıldı");
    }
}
