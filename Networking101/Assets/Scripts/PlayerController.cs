using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class CustomMsgType
{
    public static short Transform = MsgType.Highest + 1;
};


public class PlayerController : NetworkBehaviour
{
    public float m_linearSpeed = 5.0f;
    public float m_angularSpeed = 3.0f;
    public float m_jumpSpeed = 0.0f;

    public bool attackBuff = false;
    public bool defenseBuff = false;

    [SyncVar]
    public bool flagAttached = false;

    private Rigidbody m_rb = null;

    public bool IsHost()
    {
        return isServer && isLocalPlayer;
    }

    // Use this for initialization
    void Start()
    {
        m_rb = GetComponent<Rigidbody>();
        //Debug.Log("Start()");
        if (isServer)
        {
            Vector3 spawnPoint = new Vector3(-25.0f, 1.0f, 0.0f);
            this.transform.position = spawnPoint;
        }
        else
        {
            Vector3 spawnPoint = new Vector3(25.0f, 1.0f, 0.0f);
            this.transform.position = spawnPoint;
        }


    }

    public override void OnStartAuthority()
    {
        base.OnStartAuthority();
        //Debug.Log("OnStartAuthority()");
    }

    public override void OnStartClient()
    {
        base.OnStartClient();
        //Debug.Log("OnStartClient()");
    }

    public override void OnStartLocalPlayer()
    {
        base.OnStartLocalPlayer();
        //Camera.main.transform.parent = this.transform;
        //Camera.main.GetComponent<CamFollow>().target = this.transform;
        //Debug.Log("OnStartLocalPlayer()");
        GetComponent<MeshRenderer>().material.color = new Color(0.0f, 1.0f, 0.0f);
    }

    public override void OnStartServer()
    {
        base.OnStartServer();
        //Debug.Log("OnStartServer()");
    }

    public void Jump()
    {
        Vector3 jumpVelocity = Vector3.up * m_jumpSpeed;
        m_rb.velocity += jumpVelocity;
    }

    [ClientRpc]
    public void RpcJump()
    {
        Jump();
    }

    [Command]
    public void CmdJump()
    {
        Jump();
        RpcJump();
    }

    // Update is called once per frame
    void Update()
    {
        if (attackBuff == true)
        {
            StartCoroutine("AttackBuff");
        }
        if (defenseBuff == true)
        {
            StartCoroutine("DefenseBuff");
        }
        if (!isLocalPlayer)
        {
            return;
        }

        float rotationInput = Input.GetAxis("Horizontal");
        float forwardInput = Input.GetAxis("Vertical");

        Vector3 linearVelocity = this.transform.forward * (forwardInput * m_linearSpeed);

        if (Input.GetKeyDown(KeyCode.Space))
        {
            CmdJump();
        }

        float yVelocity = m_rb.velocity.y;


        linearVelocity.y = yVelocity;
        m_rb.velocity = linearVelocity;

        Vector3 angularVelocity = this.transform.up * (rotationInput * m_angularSpeed);
        m_rb.angularVelocity = angularVelocity;



    }

    public IEnumerator AttackBuff()
    {
        m_jumpSpeed = 6.0f;
        yield return new WaitForSeconds(10.0f);
        m_jumpSpeed = 0.0f;
        GetComponent<ParticleSystem>().Stop();
        attackBuff = false;
    }
    public IEnumerator DefenseBuff()
    {
        m_linearSpeed = 10.0f;
        yield return new WaitForSeconds(10.0f);
        m_linearSpeed = 5.0f;
        GetComponent<ParticleSystem>().Stop();
        defenseBuff = false;
    }

    public void OnTriggerEnter(Collider c)
    {
        Debug.Log("test1");
        if (c.gameObject.tag == "Player")
        {
            Debug.Log("test2");
            if (GameObject.FindGameObjectWithTag("flag") != null)
            {
                GameObject.FindGameObjectWithTag("flag").GetComponent<Flag>().m_state = Flag.State.Removed;
                flagAttached = false;
            }
        }
    }
}
