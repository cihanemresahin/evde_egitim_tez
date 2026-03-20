using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR;

/*
 * InputReader.cs — M3: Meta Quest Controller + Head-Tracking
 *
 * Quest controller thumbstick: UnityEngine.XR.InputDevices ile okuma
 * HMD yön: XRNode.Head rotasyonu
 */
public class InputReader : MonoBehaviour
{
    [Header("Debug — kaç frame'de bir log")]
    public int logInterval = 10;

    // M4'te ControlSender bu property'lere erişecek
    public float JoystickX { get; private set; }
    public float JoystickY { get; private set; }
    public float HeadYaw   { get; private set; }
    public float HeadPitch { get; private set; }

    private int _frameCount = 0;
    private InputDevice _leftController;

    void OnEnable()
    {
        // Controller bağlantısı değişince yeniden bul
        InputDevices.deviceConnected    += OnDeviceConnected;
        InputDevices.deviceDisconnected += OnDeviceDisconnected;
        FindLeftController();
    }

    void OnDisable()
    {
        InputDevices.deviceConnected    -= OnDeviceConnected;
        InputDevices.deviceDisconnected -= OnDeviceDisconnected;
    }

    void OnDeviceConnected(InputDevice device)    => FindLeftController();
    void OnDeviceDisconnected(InputDevice device) => FindLeftController();

    void FindLeftController()
    {
        var devices = new List<InputDevice>();
        InputDevices.GetDevicesWithCharacteristics(
            InputDeviceCharacteristics.Controller | InputDeviceCharacteristics.Left,
            devices);
        _leftController = devices.Count > 0 ? devices[0] : default;

        if (_leftController.isValid)
            Debug.Log($"[INPUT] Sol controller bulundu: {_leftController.name}");
        else
            Debug.LogWarning("[INPUT] Sol controller bulunamadı — Quest Link aktif mi?");
    }

    void Update()
    {
        // ── 1. SOL THUMBSTICK ──────────────────────────────────
        if (_leftController.isValid)
        {
            _leftController.TryGetFeatureValue(
                CommonUsages.primary2DAxis, out Vector2 stick);
            JoystickX = stick.x;   // Sola:-1  Sağa:+1
            JoystickY = stick.y;   // Geri:-1  İleri:+1
        }

        // ── 2. HEAD TRACKING ───────────────────────────────────
        var nodeStates = new List<XRNodeState>();
        InputTracking.GetNodeStates(nodeStates);
        foreach (var state in nodeStates)
        {
            if (state.nodeType == XRNode.Head &&
                state.TryGetRotation(out Quaternion rot))
            {
                Vector3 e = rot.eulerAngles;
                HeadYaw   = NormalizeAngle(e.y);
                HeadPitch = NormalizeAngle(e.x);
                break;
            }
        }

        // ── 3. DEBUG LOG ───────────────────────────────────────
        if (++_frameCount >= logInterval)
        {
            _frameCount = 0;
            Debug.Log($"[INPUT] Joystick x:{JoystickX:F2} y:{JoystickY:F2} " +
                      $"| Head yaw:{HeadYaw:F1} pitch:{HeadPitch:F1}");
        }
    }

    float NormalizeAngle(float a) => a > 180f ? a - 360f : a;
}
