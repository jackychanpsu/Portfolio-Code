using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class PlayerControl : MonoBehaviour
{
    //Initial Variables
    private Rigidbody2D rb;
    private Animator anim;
    private Collider2D coll;

    
    //FSM
    private enum State { idle, running, jumping, falling, hurt };
    private State state = State.idle;

    //Inspector Variables
    [SerializeField] private LayerMask ground;
    [SerializeField] private float speed = 5f;
    [SerializeField] private float jumpforce = 10f;
    [SerializeField] private int gems = 0;
    [SerializeField] private Text gemText;
    [SerializeField] private int health = 100;
    [SerializeField] private int VerticalHurtForce = 5;
    [SerializeField] private int HorizontalHurtForce = 3;
    [SerializeField] private Text healthText;
    [SerializeField] private AudioSource gemSound;
    [SerializeField] private AudioSource grassStepSound;

    private void Start()
    
    {
        rb = GetComponent<Rigidbody2D>();
        anim = GetComponent<Animator>();
        coll = GetComponent<Collider2D>();
        
    }

    private void Update()

    {
        if (state != State.hurt)
        {
            Movement();
        }
        
        AnimationState();
        anim.SetInteger("State", (int)state); //Sets animation on enum states
    }
    private void OnTriggerEnter2D(Collider2D collision)
    {
        if (collision.tag == "Collectable")
        {
            gemSound.Play();
            Destroy(collision.gameObject);
            gems += 1;
            gemText.text = gems.ToString();

        }
    }
    private void OnCollisionEnter2D(Collision2D collision)
    {
        if (collision.gameObject.tag == "Enemy")
        {
            if (state == State.falling)
            {
                Enemy enemy = collision.gameObject.GetComponent<Enemy>();
                enemy.Stomped();
                Jump();
            }
            else
            {
                if (collision.gameObject.transform.position.x > transform.position.x)
                //Enemy is to the right, damaged to the left
                {
                    
                    rb.velocity = new Vector2(-HorizontalHurtForce, VerticalHurtForce);
                    state = State.hurt;
                    healthText.text = health.ToString();
                }
                else
                {

                    rb.velocity = new Vector2(HorizontalHurtForce, VerticalHurtForce);
                    state = State.hurt;
                    healthText.text = health.ToString();
                }
            }
            
        }
    }



    private void Movement()
    {
        float Hdirection = Input.GetAxis("Horizontal");
        float Vdirection = Input.GetAxis("Vertical");

        if (Hdirection < 0)
        //Player is moving left
        {
            rb.velocity = new Vector2(-speed, rb.velocity.y);
            transform.localScale = new Vector2(-1, 1);

        }
        else if (Hdirection > 0)
        //Player is moving right
        {
            rb.velocity = new Vector2(speed, rb.velocity.y);
            transform.localScale = new Vector2(1, 1);

        }

        if (Input.GetButtonDown("Jump") && coll.IsTouchingLayers(ground))
        //Player is jumping
        {
            Jump();
        }
    }

    private void Jump()
    {
        state = State.jumping;
        rb.velocity = new Vector2(rb.velocity.x, jumpforce);
    }
    private void AnimationState()
    {
        if (state == State.jumping)
        {
            if (rb.velocity.y < .1f)
                //Player is falling
            {
                state = State.falling;
            }
        }

        else if (state == State.falling)
        {
            if (coll.IsTouchingLayers(ground))
                //Player stops falling
            {
                state = State.idle;
            }
        }
        else if (state == State.hurt)
        {
            
            if (Mathf.Abs(rb.velocity.x) < .1f)
            {
                health -= 10;
                state = State.idle;
            }
        }
        else if (Mathf.Abs(rb.velocity.x) > 1f)
            //Character moving
        {
            state = State.running;
        }
        else
        {
            state = State.idle;
        }

    }
    private void WalkSound()
    {
        grassStepSound.Play();
    }
}

