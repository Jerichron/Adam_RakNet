using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class JumpPowerUp : NetworkBehaviour
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
    public void RpcPickUpPowerA(GameObject player)
    {
        ActivatePowerUpA(player);
    }

    public void ActivatePowerUpA(GameObject obj)
    {
        obj.GetComponent<PlayerController>().jumpActive = true;
        ParticleSystem p = obj.GetComponent<ParticleSystem>();
        var main = p.main;
        main.startColor = Color.red;
        p.Play();
    }

    void OnTriggerEnter(Collider c)
    {
        if (c.tag == "Player")
        {
            m_state = State.Possessed;
            ActivatePowerUpA(c.gameObject);
            RpcPickUpPowerA(c.gameObject);
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
