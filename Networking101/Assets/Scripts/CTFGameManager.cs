using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

public class CTFGameManager : NetworkBehaviour
{

    public int m_numPlayers = 2;

    public GameObject[] Players;
    public GameObject m_flag = null;
    public GameObject m_JumpPowerUp = null;
    public GameObject m_SpeedPowerUp = null;
    public Text timeleft;
    public Text Score;
    public Text WinLose;

    public enum CTF_GameState
    {
        GS_WaitingForPlayers,
        GS_Ready,
        GS_InGame,
    }

    [SyncVar]
    CTF_GameState m_gameState = CTF_GameState.GS_WaitingForPlayers;

    [SyncVar]
    public float timeRemaining = 30.0f;
    [SyncVar]
    public float P1Score = 0.0f;
    [SyncVar]
    public float P2Score = 0.0f;

    public bool SpawnFlag()
    {
        GameObject flag = Instantiate(m_flag, new Vector3(0, 0, 0), new Quaternion());
        NetworkServer.Spawn(flag);
        return true;
    }

    public void SpawnPowerUps()
    {
        //Jump power up.
        if (GameObject.FindGameObjectWithTag("Jump") == null)
        {
            int r = Random.Range(1, 4);
            Vector3 spawnLoc = new Vector3();
            switch (r)
            {
                case 1:
                    spawnLoc = new Vector3(35.0f, 4.0f, 35.0f);
                    break;
                case 2:
                    spawnLoc = new Vector3(-35.0f, 4.0f, -35.0f);
                    break;
                case 3:
                    spawnLoc = new Vector3(35.0f, 4.0f, -35.0f);
                    break;
                case 4:
                    spawnLoc = new Vector3(-35.0f, 4.0f, 35.0f);
                    break;
                default:
                    Debug.Log("Error");
                    break;
            }
            GameObject JumpPowerUp = Instantiate(m_JumpPowerUp, spawnLoc, new Quaternion());
            NetworkServer.Spawn(JumpPowerUp);
        }
        //Speed power up.
        if (GameObject.FindGameObjectWithTag("Speed") == null)
        {
            int r = Random.Range(1, 4);
            Vector3 spawnLoc = new Vector3();
            switch (r)
            {
                case 1:
                    spawnLoc = new Vector3(35.0f, 0.5f, 0.0f);
                    break;
                case 2:
                    spawnLoc = new Vector3(-35.0f, 0.5f, 0.0f);
                    break;
                case 3:
                    spawnLoc = new Vector3(0.0f, 0.5f, -35.0f);
                    break;
                case 4:
                    spawnLoc = new Vector3(0.0f, 0.5f, 35.0f);
                    break;
                default:
                    Debug.Log("Error");
                    break;
            }
            GameObject SpeedPowerUp = Instantiate(m_SpeedPowerUp, spawnLoc, new Quaternion());
            NetworkServer.Spawn(SpeedPowerUp);
        }
    }
    bool IsNumPlayersReached()
    {
        return CTFNetworkManager.singleton.numPlayers == m_numPlayers;
    }

    // Use this for initialization
    void Start()
    {
        WinLose.enabled = false;
    }

    // Update is called once per frame
    void Update()
    {
        if (isServer)
        {
            if (m_gameState == CTF_GameState.GS_WaitingForPlayers && IsNumPlayersReached())
            {
                m_gameState = CTF_GameState.GS_Ready;
            }

            if (m_gameState == CTF_GameState.GS_InGame)
            {
                timeRemaining -= Time.deltaTime;
                if (timeRemaining <= 0)
                {
                    timeRemaining = 0.0f;
                }
                timeleft.text = "Time Left: " + Mathf.RoundToInt(timeRemaining).ToString();
                if (P1Score > P2Score)
                {
                    Score.text = "1st Place";
                }
                else if (P2Score > P1Score)
                {
                    Score.text = "2nd Place";
                }

                if (timeRemaining == 0)
                {
                    WinLose.enabled = true;
                    if (P2Score < P1Score)
                    {
                        WinLose.text = "You Win";
                    }
                    else if (P1Score < P2Score)
                    {
                        WinLose.text = "You Lose";
                    }
                }
            }
        }
        else
        {
            if (m_gameState == CTF_GameState.GS_InGame)
            {
                timeleft.text = "Time Left: " + Mathf.RoundToInt(timeRemaining).ToString();
                if (P2Score > P1Score)
                {
                    Score.text = "1st Place";
                }
                else if (P1Score > P2Score)
                {
                    Score.text = "2nd Place";
                }

                if (timeRemaining == 0)
                {
                    WinLose.enabled = true;
                    if (P2Score > P1Score)
                    {
                        WinLose.text = "You Win";
                    }
                    else if (P1Score > P2Score)
                    {
                        WinLose.text = "You Lose";
                    }
                }
            }
        }

        if (m_gameState == CTF_GameState.GS_InGame)
        {
            foreach (GameObject player in Players)
            {
                if (player.GetComponent<PlayerController>().IsHost() && player.GetComponent<PlayerController>().flagAttached == true)
                {
                    P1Score += Time.deltaTime;
                }
                else if (!player.GetComponent<PlayerController>().IsHost() && player.GetComponent<PlayerController>().flagAttached == true)
                {
                    P2Score += Time.deltaTime;
                }
            }
        }
        UpdateGameState();
    }

    public void UpdateGameState()
    {
        if (m_gameState == CTF_GameState.GS_Ready)
        {
            //call whatever needs to be called
            if (isServer)
            {
                SpawnFlag();
                InvokeRepeating("SpawnPowerUps", 0.1f, 10.0f);
                Players = GameObject.FindGameObjectsWithTag("Player");
                //Start game
                m_gameState = CTF_GameState.GS_InGame;
            }
        }
    }
}
