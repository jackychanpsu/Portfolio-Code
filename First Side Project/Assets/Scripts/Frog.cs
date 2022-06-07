using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Frog : Enemy
{
    private Collider2D coll;




    [SerializeField] LayerMask ground;
    [SerializeField] private float leftBoundary;
    [SerializeField] private float rightBoundary;
    [SerializeField] private float jumpLength = 3f;
    [SerializeField] private float jumpHeight = 5f;


    private bool facingLeft = true;

    protected override void Start()
    {
        base.Start();
        coll = GetComponent<Collider2D>();
        
    }

    private void Update()
    {
        if (anim.GetBool("Jumping") == true)
        {
            if (rb.velocity.y < .1)
            {
                anim.SetBool("Jumping", false);
                anim.SetBool("Falling", true);
            }

        }
        if (coll.IsTouchingLayers(ground) && anim.GetBool("Falling"))
        {
            anim.SetBool("Falling", false);
        }
    }

    private void Move()
    {
        if (facingLeft)
        {
            if (transform.position.x > leftBoundary)
            {
                if (transform.localScale.x != 1)
                {
                    transform.localScale = new Vector3(1, 1);
                }
                if (coll.IsTouchingLayers(ground))
                {
                    rb.velocity = new Vector2(-jumpLength, jumpHeight);
                    anim.SetBool("Jumping", true);
                }
            }
            else
            {
                facingLeft = false;
            }

        }
        else
        {
            if (transform.position.x < rightBoundary)
            {
                if (transform.localScale.x != -1)
                {
                    transform.localScale = new Vector3(-1, 1);
                }
                if (coll.IsTouchingLayers(ground))
                {
                    rb.velocity = new Vector2(jumpLength, jumpHeight);
                    anim.SetBool("Jumping", true);
                }
            }
            else
            {
                facingLeft = true;
            }
        }
    }
    
    
}

