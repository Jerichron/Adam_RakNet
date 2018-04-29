using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

public class CTFNetworkManager : NetworkManager
{
    public Canvas StartScreen;
    public Button Host_Game;
    public Button Join_Game;
    public Button Quit_Game;
    public GameObject Text;

    public void Start()
    {
        Text.SetActive(false);
    }

    public void StartupHost()
    {
        SetPort();
        NetworkManager.singleton.StartHost();
    }
    public void JoinGame()
    {
        SetPort();
        NetworkManager.singleton.StartClient();
    }
    public void QuitGame()
    {
        Application.Quit();
    }
    public void Tutorial()
    {
        Text.SetActive(true);
    }
    void SetPort()
    {
        NetworkManager.singleton.networkPort = 7777;
    }
    public override void OnServerConnect(NetworkConnection conn)
    {
        base.OnServerConnect(conn);
        Debug.Log("OnServerConnect NumPlayers:" + this.numPlayers);
        StartScreen.enabled = false;
        Host_Game.enabled = false;
        Join_Game.enabled = false;
    }

    public override void OnClientConnect(NetworkConnection conn)
    {
        base.OnClientConnect(conn);
        Debug.Log("OnClientConnect NumPlayers:" + this.numPlayers);
        StartScreen.enabled = false;
        Host_Game.enabled = false;
        Join_Game.enabled = false;
    }

    public override void OnClientDisconnect(NetworkConnection conn)
    {
        base.OnClientDisconnect(conn);
        Debug.Log("OnClientDisconnect NumPlayers" + this.numPlayers);
    }

    //public override void OnServerAddPlayer(NetworkConnection conn, short playerControllerId, NetworkReader extraMessageReader)
    //{
    //m_gameManager.ActivePlayers = this.numPlayers;
    //}
}
