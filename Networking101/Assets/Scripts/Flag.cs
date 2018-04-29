using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class Flag : NetworkBehaviour
{
    public enum State
    {
        Available,
        Possessed,
        Removed
    };

    [SyncVar]
    public State m_state;

    // Use this for initialization
    void Start()
    {
        m_state = State.Available;
    }

    [ClientRpc]
    public void RpcPickUpFlag(GameObject player)
    {
        AttachFlagToGameObject(player);
    }

    public void AttachFlagToGameObject(GameObject obj)
    {
        this.transform.parent = obj.transform;
        this.transform.localPosition = new Vector3(0.5f, 0.0f, -0.8f);
        obj.GetComponent<PlayerController>().flagAttached = true;

    }


    void OnTriggerEnter(Collider other)
    {
        if (m_state == State.Possessed || !isServer || other.tag != "Player")
        {
            this.GetComponent<ParticleSystem>().Stop();
            return;
        }

        m_state = State.Possessed;
        AttachFlagToGameObject(other.gameObject);
        RpcPickUpFlag(other.gameObject);

    }

    // Update is called once per frame
    void Update()
    {
        if (m_state == State.Removed)
        {
            this.transform.parent = null;
            this.transform.position = Vector3.zero;
            m_state = State.Available;
        }
        if (m_state == State.Possessed && this.GetComponent<ParticleSystem>().isPlaying == true)
        {
            this.GetComponent<ParticleSystem>().Stop();
        }
        if (m_state == State.Available && this.GetComponent<ParticleSystem>().isPlaying == false)
        {
            this.GetComponent<ParticleSystem>().Play();
        }
    }
}
