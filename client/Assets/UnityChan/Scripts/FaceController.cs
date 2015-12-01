using System.Collections;
using UnityEngine;

using CnControls;
using unitychan_messages;


public class FaceController : MonoBehaviour
{
	public AnimationClip[] animations;
    public bool itsMe = true;

	private Animator anim;


    void Start ()
	{
		anim = GetComponent<Animator> ();
	}

	void Update ()
    {
        if (!itsMe)
            return;

        int index = -1;

        if (CnInputManager.GetButtonDown("Smile")) {
            index = 1;
        }
        else if (CnInputManager.GetButtonDown("Bad")) {
            index = 2;
        }
        else if (CnInputManager.GetButtonDown("Angry")) {
            index = 3;
        }
        if (CnInputManager.GetButtonUp("Smile") ||
            CnInputManager.GetButtonUp("Bad") ||
            CnInputManager.GetButtonUp("Angry")) {
            index = 0;
        }

        if (index != -1)
        {
            if (index == 0) {
                NetworkController.instance.SendChangeFace("default@unitychan", 0);
            }
            else if (index == 1) {
                NetworkController.instance.SendChangeFace("smile1@unitychan", 1);
            }
            else if (index == 2) {
                NetworkController.instance.SendChangeFace("disstract1@unitychan", 1);
            }
            else if (index == 3) {
                NetworkController.instance.SendChangeFace("angry2@unitychan", 1);
            }
        }
    }

    public void ChangeFace (FaceUpdate face)
    {
        anim.CrossFade(face.clip, 0);
        anim.SetLayerWeight(face.layer, 1f);
    }
}
