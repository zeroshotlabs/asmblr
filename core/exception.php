<?php declare(strict_types=1);
namespace asm\_e;

use function \asm\sys\_stde;
use asm\http\http_headers;
use asm\types\url;


/**
 * Namespace-local exception.
 *
 * Thrown internally by asmblr and can send HTTP headers and log
 * appropriately.  They can also redirect or indicate a 404 -
 * even on the CLI!
 * 
 * @see eHTTP
 */
class exception extends \Exception
{
    use http_headers;

    protected bool $IsCLI = false;

    public function __construct( string $message,int $code = 0,\Throwable $previous = null )
    {
        parent::__construct($message,$code,$previous);

        if( !empty($_SERVER['argv']) )
            $this->IsCLI = true;
    }

    public function __toString(): string
    {
        if( $this->IsCLI )
        {
            _stde("EXCEPTION: ({$this->code}) {$this->message}");
        }
        else
        {
            _stde("EXCEPTION: ({$this->code}) {$this->message}");
        }

        return "EXCEPTION: ({$this->code}) {$this->message}";
    }
}


/**
 * asmblr pairs exceptions with HTTP error codes, even on the CLI.
 * 
 * In a web runtime, they are used to control user interaction,
 * and will send appropriate HTTP headers, change templates, as well as
 * log, notify, etc.  Logging uses the SAPI's standard error handler.
 * 
 * On the CLI, they indicate a kindred error condition, and will act
 * appropriately, which includes not sending HTTP headers.  Output will
 * be based on STDOUT and STDERR, or additional log files, for instance
 * when using _d.
 * 
 * Applications should extend these to perform custom handling, such
 * as swapping in templates/renders.
 * 
 * Exceptions can also be thrown and re-thrown as needed, with or without
 * sending HTTP codes and redirecting.
 * 
 * @note $app may be in a failed state so this operates directly on the SUPER! globals.
 * @todo External (email) notifications.
 */
class eHTTP extends exception
{
    use http_headers;

    protected int $http_code = 200;
//    protected string $http_response_string = 'HTTP/1.1 200 OK';

    /**
     * Sets HTTP response code using http_response_code() with message
     * and custom handling.
     * 
     * @param string $message General purpose message, usually logged.
     * @param int $code System error code, which is generally an HTTP code.
     *                  -1 will prevent sending an HTTP response header.
     * @param \Throwable $previous Previous exception.
     * 
     * @todo $previous isn't used currently.
     */
    public function __construct( string $message = null,int $code = 0,\Throwable $previous = null )
    {
        parent::__construct((string)$message,$this->http_code,$previous);

        if( $code > -1 )
            $this->send_response_code($code>0?$code:$this->http_code);
    }
}

/** 200 OK */
class e200 extends eHTTP
{
    protected int $http_code = 200;
}

/**
 * 204 No Content (typically for OPTIONS responses).
 * 
 * @see \asm\extensions\corsp
 */
class e204 extends eHTTP
{
    protected int $http_code = 204;
}

/** 301 permanent redirect. */
class e301 extends eHTTP
{
    protected int $http_code = 301;
}

/** 302 temporary redirect. */
class e302 extends eHTTP
{
    protected int $http_code = 302;
}

/**
 * Send a Location header for redirecting.
 *
 * @param url|string  $url The URL to redirect to.
 * @param  $url The URL to redirect to.
 * @param boolean $Perm FALSE to not send a 301 header first.
 * @param boolean $Exit FALSE to not kill execution after sending the header.
 * 
 * @note Can be trown as an exception or instantiated and called as an object.
 */
class go2 extends eHTTP
{
    protected string $dest_url = '';
    protected $IsPerm = true;

    /**
     * @todo add relative/absolute redirects, external, endpoint awareness, theme/asset awareness, etc.
     * @todo option to hard exit?
     */
    public function __construct( string|url $url,int|bool $perm = true,\Throwable $previous = null )
    {
        $this->dest_url = (string) $url;
        $this->IsPerm = (bool) $perm;

        parent::__construct($this->dest_url,-1,$previous);

        if($this->IsPerm )
            $this->send_response_code(301);

        header("Location: {$this->dest_url}");

        // if( $Exit === TRUE )
        //     exit;
    }
}

/**
 * 400 Bad Request.
 * 
 * Useful on the API and CLI.
 */
class e400 extends eHTTP
{
    protected int $http_code = 400;
}

/** 401 Unauthorized. */
class e401 extends eHTTP
{
    protected int $http_code = 401;
}

/** 403 Forbidden. */
class e403 extends eHTTP
{
    protected int $http_code = 403;
}

/**
 * 404 Not Found.
 * 
 * Typical 404 error.  Should generally swap in the approprite template.
 */
class e404 extends eHTTP
{
    protected int $http_code = 404;
}

/**
 * 500 Internal Server Error.
 * 
 * Unrecoverable error.  Usually outputs generic/static notice.
 * 
 * Default internal asmblr exception.
 * 
 * Should be used for CLI apps.
 */
class e500 extends eHTTP
{
    protected int $http_code = 500;
}

/** 
 * 501 Not Implemented.
 * 
 * Generally used with API apps; generally would render a
 * JSON response/error message.
 */
class e501 extends eHTTP
{
    protected int $http_code = 501;
}

/** 503 Service Unavailable. */
class e503 extends eHTTP
{
    protected int $http_code = 503;
}
