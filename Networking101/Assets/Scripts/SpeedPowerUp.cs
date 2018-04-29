using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class SpeedPowerUp : NetworkBehaviour
{
    enum State
    {
        Available,
        Possessed
    };

    [SyncVar]
    State m_state;

    // Use this for initialization
    void Start()
    {
        m_state = State.Available;
    }

    [ClientRpc]
    public void RpcPickUpPowerD(GameObject player)
    {
        ActivatePowerUpD(player);
    }

    public void ActivatePowerUpD(GameObject obj)
    {
        obj.GetComponent<PlayerController>().speedActive = true;
        ParticleSystem p = obj.GetComponent<ParticleSystem>();
        var main = p.main;
        main.startColor = Color.blue;
        p.Play();
    }

    void OnTriggerEnter(Collider c)
    {
        if (c.tag == "Player")
        {
            m_state = State.Possessed;
            ActivatePowerUpD(c.gameObject);
            RpcPickUpPowerD(c.gameObject);
        }
    }

    void Update()
    {
        if (m_state == State.Possessed)
        {
            Destroy(this.gameObject);
        }
    }
}
