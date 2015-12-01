using Fun;
using ProtoBuf;
using UnityEngine;
using System.Collections.Generic;

using funapi.network.fun_message;
using funapi.service.multicast_message;
using unitychan_messages;


public class NetworkController : MonoBehaviour
{
    private const string kServerIp = "127.0.0.1";
    public static NetworkController instance;
    public delegate void FaceUpdateHandler(FaceUpdate state);
    public delegate void InputUpdateHandler(UserState state);

    public GameObject m_ChanPrefab;
    public GameObject m_ChanOtherPrefab;

    private FunapiNetwork network;
    private int my_id;
    private GameObject my_chan;
    private Dictionary<int, FaceUpdateHandler> face_callbacks = new Dictionary<int, FaceUpdateHandler>();
    private Dictionary<int, InputUpdateHandler> input_callbacks = new Dictionary<int, InputUpdateHandler>();


    void Awake ()
    {
        instance = gameObject.GetComponent<NetworkController>();

        network = new FunapiNetwork(false);
        network.OnSessionInitiated += new FunapiNetwork.SessionInitHandler(OnSessionInitiated);
        network.OnSessionClosed += new FunapiNetwork.SessionCloseHandler(OnSessionClosed);

        FunapiTcpTransport transport = new FunapiTcpTransport(kServerIp, 8022, FunEncoding.kProtobuf);
        transport.AutoReconnect = true;
        network.AttachTransport(transport);

        network.RegisterHandler("sc_spawn_self", this.OnSpawnSelf);
        network.RegisterHandler("sc_spawn_other", this.OnSpawnOther);
        network.RegisterHandler("sc_update", this.OnUpdate);
        network.RegisterHandler("sc_killed", this.OnKilled);
        network.RegisterHandler("sc_face", this.OnFaceUpdate);
    }

    // Use this for initialization
    void Start ()
    {
        network.Start();
    }

    // Update is called once per frame
    void Update ()
    {
        if (network != null)
            network.Update();
    }

    void OnApplicationQuit()
    {
        if (network != null)
            network.Stop();
    }

    public int MyId
    {
        get { return my_id; }
    }

    public GameObject GetMyPlayer ()
    {
        return my_chan;
    }

    private void OnSessionInitiated (string session_id)
    {
    }

    private void OnSessionClosed ()
    {
        network = null;
    }

    public void SendInput (InputUpdate input)
    {
        FunMessage message = network.CreateFunMessage(input, MessageType.cs_input);
        network.SendMessage(MessageType.cs_input, message);
    }

    public void SendChangeFace (string clip, int layer)
    {
        ChangeFace face = new ChangeFace();
        face.clip = clip;
        face.layer = layer;

        FunMessage message = network.CreateFunMessage(face, MessageType.cs_face);
        network.SendMessage(MessageType.cs_face, message);
    }

    private void SpawnPlayer (int id, Vector3 pos)
    {
        GameObject player;
        if (id == my_id) {
            player = Instantiate(m_ChanPrefab) as GameObject;
            my_chan = player;
        }
        else {
            player = Instantiate(m_ChanOtherPrefab) as GameObject;
        }

        player.name = "player " + id;
        player.transform.position = pos;

        FaceController fc = player.GetComponent<FaceController>();
        face_callbacks.Add(id, fc.ChangeFace);

        UnityChanControlScriptWithRgidBody uc = player.GetComponent<UnityChanControlScriptWithRgidBody>();
        input_callbacks.Add(id, uc.UpdatePosition);

        DebugUtils.Log("spawn player {0} - pos:{1}", id, pos);
    }

    private void OnSpawnSelf (string msg_type, object body)
    {
        FunMessage msg = body as FunMessage;
        object obj = network.GetMessage(msg, MessageType.sc_spawn_self);
        SpawnSelf spawn = obj as SpawnSelf;

        my_id = spawn.me.user_id;
        SpawnPlayer(spawn.me.user_id, new Vector3(spawn.me.p_x, 0f, spawn.me.p_z));

        foreach (UserState s in spawn.others)
        {
            SpawnPlayer(s.user_id, new Vector3(s.p_x, 0f, s.p_z));
        }

        Camera.main.gameObject.AddComponent(typeof(ThirdPersonCamera));
    }

    private void OnSpawnOther (string msg_type, object body)
    {
        FunMessage msg = body as FunMessage;
        object obj = network.GetMessage(msg, MessageType.sc_spawn_other);
        Spawn spawn = obj as Spawn;

        SpawnPlayer(spawn.user_id, new Vector3(spawn.p_x, 0f, spawn.p_z));
    }

    private void OnUpdate (string msg_type, object body)
    {
        FunMessage msg = body as FunMessage;
        object obj = network.GetMessage(msg, MessageType.sc_update);
        UserUpdate list = obj as UserUpdate;

        foreach (UserState s in list.users)
        {
            if (input_callbacks.ContainsKey(s.user_id))
                input_callbacks[s.user_id](s);
        }
    }

    private void OnKilled (string msg_type, object body)
    {
        FunMessage msg = body as FunMessage;
        object obj = network.GetMessage(msg, MessageType.sc_killed);
        Killed killed = obj as Killed;

        if (killed.user_id == my_id) {
            my_id = -1;
            my_chan = null;
        }

        string name = "player " + killed.user_id;
        GameObject player = GameObject.Find(name);
        if (player != null)
            Destroy(player);

        if (face_callbacks.ContainsKey(killed.user_id))
            face_callbacks.Remove(killed.user_id);

        if (input_callbacks.ContainsKey(killed.user_id))
            input_callbacks.Remove(killed.user_id);

        DebugUtils.Log("Received killed message - user_id:{0}", killed.user_id);
    }

    private void OnFaceUpdate (string msg_type, object body)
    {
        FunMessage msg = body as FunMessage;
        object obj = network.GetMessage(msg, MessageType.sc_face);
        FaceUpdate face = obj as FaceUpdate;

        if (face_callbacks.ContainsKey(face.user_id))
            face_callbacks[face.user_id](face);
    }
}
