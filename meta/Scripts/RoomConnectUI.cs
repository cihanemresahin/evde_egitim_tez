using UnityEngine;
using UnityEngine.UI;
using TMPro;

/*
 * RoomConnectUI.cs — M10: Oda Kodu Giriş Ekranı
 *
 * Öğrenci 6 haneli oda kodunu girer, "Bağlan" butonuna basar.
 * Bağlantı sağlanınca UI gizlenir, VR ortamı aktif olur.
 *
 * Kurulum:
 *   1. Canvas → Panel oluştur (arka plan)
 *   2. TMP_InputField + Button ekle
 *   3. Bu scripti Canvas'a ekle, field'ları ata
 */
public class RoomConnectUI : MonoBehaviour
{
    [Header("UI Elemanları")]
    public TMP_InputField codeInput;
    public Button connectButton;
    public TextMeshProUGUI statusText;
    public GameObject connectPanel;  // giriş paneli

    [Header("Referanslar")]
    public ControlSender controlSender;

    void Start()
    {
        connectButton.onClick.AddListener(OnConnect);
        if (statusText != null)
            statusText.text = "Oda kodunu girin";
    }

    void OnConnect()
    {
        string code = codeInput.text.Trim();
        if (code.Length != 6)
        {
            if (statusText != null)
                statusText.text = "6 haneli kod girin";
            return;
        }

        connectButton.interactable = false;
        if (statusText != null)
            statusText.text = "Bağlanıyor...";

        controlSender.Connect(code);
    }

    void Update()
    {
        if (statusText != null)
            statusText.text = ControlSender.Status;

        // Bağlantı sağlandıysa paneli gizle
        if (ControlSender.IsConnected && connectPanel != null)
        {
            connectPanel.SetActive(false);
        }
        // Bağlantı koptuysa paneli göster
        else if (!ControlSender.IsConnected && connectPanel != null && !connectPanel.activeSelf)
        {
            connectPanel.SetActive(true);
            connectButton.interactable = true;
        }
    }
}
