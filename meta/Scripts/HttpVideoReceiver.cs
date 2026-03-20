using UnityEngine;

/*
 * HttpVideoReceiver.cs — M10: WebSocket Video Alıcı
 *
 * ControlSender'dan gelen binary JPEG frame'leri
 * texture'e dönüştürüp Quad/RawImage üzerinde gösterir.
 *
 * Not: Artık HTTP polling değil, ControlSender.OnVideoFrame
 * event'ini dinler (relay üzerinden gelen binary JPEG).
 */
public class HttpVideoReceiver : MonoBehaviour
{
    [Header("Görüntü — birini doldur")]
    public UnityEngine.UI.RawImage displayImage;
    public Renderer curvedRenderer;

    [Header("Debug")]
    public int logInterval = 300; // kaç frame'de bir log

    private Texture2D _tex;
    private byte[] _lastJpeg;
    private bool _hasNewFrame = false;
    private readonly object _lock = new object();
    private int _frameCount = 0;

    void OnEnable()
    {
        _tex = new Texture2D(2, 2, TextureFormat.RGB24, false);
        ControlSender.OnVideoFrame += OnFrame;
    }

    void OnDisable()
    {
        ControlSender.OnVideoFrame -= OnFrame;
    }

    void OnFrame(byte[] jpeg)
    {
        lock (_lock)
        {
            _lastJpeg = jpeg;
            _hasNewFrame = true;
        }
    }

    void Update()
    {
        if (!_hasNewFrame) return;

        byte[] jpeg;
        lock (_lock)
        {
            jpeg = _lastJpeg;
            _hasNewFrame = false;
        }

        if (jpeg == null || jpeg.Length < 100) return;

        try
        {
            _tex.LoadImage(jpeg, false);

            if (displayImage != null)
                displayImage.texture = _tex;

            if (curvedRenderer != null)
                curvedRenderer.material.mainTexture = _tex;

            _frameCount++;
            if (_frameCount % logInterval == 0)
                Debug.Log($"[VIDEO] {_frameCount} frame alındı");
        }
        catch (System.Exception e)
        {
            Debug.LogWarning($"[VIDEO] Texture hatası: {e.Message}");
        }
    }
}
