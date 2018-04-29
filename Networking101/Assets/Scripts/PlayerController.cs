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

    public bool speedActive = false;
    public bool jumpActive = false;

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
    }

    public override void OnStartClient()
    {
        base.OnStartClient();
    }

    public override void OnStartLocalPlayer()
    {
        base.OnStartLocalPlayer();
        GetComponent<MeshRenderer>().material.color = new Color(0.0f, 1.0f, 0.0f);
    }

    public override void OnStartServer()
    {
        base.OnStartServer();
    }

    public void Jump()
    {
        if (m_rb.velocity.y == 0.0f)
        {
            Vector3 jumpVelocity = Vector3.up * m_jumpSpeed;
            m_rb.velocity += jumpVelocity;
        }
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
        if (speedActive == true)
        {
            StartCoroutine("DefenseBuff");
        }
        if (jumpActive == true)
        {
            StartCoroutine("AttackBuff");
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
        m_jumpSpeed = 10.0f;
        yield return new WaitForSeconds(10.0f);
        m_jumpSpeed = 0.0f;
        GetComponent<ParticleSystem>().Stop();
        speedActive = false;
    }
    public IEnumerator DefenseBuff()
    {
        m_linearSpeed = 10.0f;
        yield return new WaitForSeconds(10.0f);
        m_linearSpeed = 5.0f;
        GetComponent<ParticleSystem>().Stop();
        jumpActive = false;
    }

    public void OnTriggerEnter(Collider c)
    {
        if (c.gameObject.tag == "Player")
        {
            if (GameObject.FindGameObjectWithTag("Flag") != null)
            {
                GameObject.FindGameObjectWithTag("Flag").GetComponent<Flag>().m_state = Flag.State.Removed;
                flagAttached = false;
            }
        }
    }
}
