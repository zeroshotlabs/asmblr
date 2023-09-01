<?php declare(strict_types=1);
namespace asm\E;


/**
 * Namespace-local exception.
 *
 * Thrown internally by asmblr.
 */
class exception extends \Exception
{
}


class eHTTP extends exception
{
    
}
// default if otherwise uncaught
class e500 extends eHTTP
{

}

class e404 extends eHTTP
{

}

class e501 extends eHTTP
{

}

