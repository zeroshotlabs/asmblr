<?php declare(strict_types=1);
/**
 * @file types.php Base traits, interfaces and abstract classes.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License v3.0 or later.
 * @copyright See COPYRIGHT.txt.
 */
namespace asm\types;
use function asm\sys\_stde;
use asm\_e\e500;

/**
 * 
 */
trait encoded_str
{
 //   public str_encodings $_encoding;


    /**
     * Create a new encoded string from an array.
     * 
     * @param array $arr Array of data, typically key/value pairs.
     * @return self A encoded string.
     */
    public static function arr( array $arr ): self
    {
        return new self($arr);       
    }

    // /**
    //  * Counts the number of key/value pairs.
    //  * 
    //  * @return int The number of key/value pairs.
    //  */
    // public function count(): int
    // {
    //     return count(array_keys($this));
    // }

    /**
     * Create a string from the data.
     *
     * @return string The URL encoded query string.
     *
     * @note This uses http_build_query() with $encoding which defaults to PHP_QUERY_RFC3986.
     * @note This uses arg_separator.output.
     * @note A '?' is automatically prefixed.    @todo  not done now
     */
    public function __toString(): string
    {
        return http_build_query($this->getArrayCopy(),'',null,$this->_encoding);
    }
}


// placeholder
class toke
{
    use tokenized;
}


/**
 * Enable classes to send and receive "tokens", or class-specific key/value pairs,
 * often representing configuration or containerized actions and parameters.
 * 
 * @see asm\types\token
 */
trait tokenized
{
    public array $tokens = [];
    public string|null $type = null;

    /**
     * Parses and initializes tokens for caching by config.
     * 
     * While a class can override this method to provide it's own parsing,
     * by default this parses as follows:
     *  - key=value&key1=value1 - set the key/value pairs
     *  - key:{op}:value - set the key/value pairs, performing an optional operation:
     *      - < > - prepend or append the value
     *      - = - set the value (generally same as append)
     *      - empty/null - remove the key/value pair
     * 
     * Most typically used for paths, titles, etc.
     * 
     * The value can also be a JSON string, in which case it's converted as a
     * key/value object. 
     *
     * @param array $tokens Array of raw tokens from config.
     * @return array Parsed tokens.
     * @throws e500 Unexpected token type.
     * @note While the creation of tokens are cached in config, they are applied
     *       at runtime in a class specific way.
     */
    public static function parse_tokens( array $tokens ): array
    {
        $parsed = [];
        foreach( $tokens as $k => $v )
        {
            if( !is_string($v) )
                throw new e500("Token not string for '$k'");

            if( isset($parsed[$k]) )
                throw new e500("Duplicate token '$k'");
            // json
            if( strpos($v,'{') !== false && strpos($v,'}') !== false )
                $parsed[$k] = json_decode($v,true);
            // internal change string
            else
                $parsed[$k] = array_filter(explode(':',$v));
        }

        return $parsed;
    }

    /**
     * Create a token to be used with use_token().
     * 
     * Generally this will create a token of the same type as the current
     * token type defined for the class, but doesn't have to.
     * 
     * Tokens are most typically created during configuration initialization.
     */
    public function create_token( $name,$type = '',... $args ): array
    {
        $t = $this->_generic;
        $t[0] = $name;
        $t[1] = $type??$this->type;
        $t[2] = $args;

        return $t;
    }
}




/**
 * Fork and daemonize base class.
 * 
 * Used by cliapp, though it can be used in the web runtime, too.
 */

trait _d
{
    public $pid = null;

    public $chroot = '/';

    // where the logs and socket are opened
    public $cwd;
    
    // by default the implementing class name is used as a prefix
    // set this to override
    public $prefix = null;
    
    public $stdout_path;
    public $stderr_path;

    // used as a suffix to the class name if not specified 
    public $socket_path = '_d.sock';
    // public $pipe_in_path = '_d.pipe.in';

    public $socket;
    // public $pipe_in;

    public $client_info;



    public function __construct( protected \asm\App $app,protected \asm\promptd\promptdio $pio )
    {
        // accept input via pipe; pipe output goes to a log file
        if( is_readable($this->pipe_in_path) )
        {
            _stde("Unlinking old $this->pipe_in_path");
            unlink($this->pipe_in_path);
        }

        // open apparently has to happen in the run/while loop
        if( posix_mkfifo($this->pipe_in_path,0644) === false )
        {
            _stde("Error creating named pipe at $this->pipe_in_path");
            exit(1);
        }

        if( is_readable($this->socket_path) )
        {
            _stde("Unlinking old $this->socket_path");
            unlink($this->socket_path);
        }

        $this->socket = stream_socket_server('unix://'.$this->socket_path,$errno,$errstr);
        if( !$this->socket )
        {
            _stde("Unable to bind Unix socket: $errstr ($errno)");
            exit(1);
        }
    }


    /**
     * Daemonize the current runtime, whether web or CLI.
     * 
     * Double forks, closes stdin, stdout, stderr, and sets session id to fully detach.
     * 
     * stdout/stderr are reopened to log files; stdin is not; chdir to root by default
     */
    public function daemonize()
    {
        if( $this->stdout_path === null || $this->stderr_path === null )
            throw new e500('stdout_path and stderr_path must be set before daemonizing.');

        // Daemonize doobly
        $this->pid = pcntl_fork();

        if( $this->pid === -1 )
            exit("Unable to daemonize fork 1");
        else if( $this->pid !== 0 )
            exit(0);
        
        $this->pid = pcntl_fork();

        if( $this->pid === -1 )
            exit("Unable to daemonize fork 2");
        else if( $this->pid !== 0 )
            exit(0);
    
        if( posix_setsid() === -1 )
            exit("Error: Unable to set session id.");

        chdir($this->chroot);

        // close std* and re-establish as $STDOUT, $STDERR;
        // has to be done in order and becomes vars;
        // @note however this doesn't seem very reliable;
        // @PHP maybe I'm missing something but this should be cleaner
        fclose(STDIN);
        fclose(STDOUT);
        fclose(STDERR);
    
        // re-establish stdout - use _stdo()/_stde() from here on out
        $GLOBALS['STDOUT'] = fopen($this->stdout_path,'a');
    
        // @note messing with STDERR didn't work - probably some startup error thing
        // this seems like the best bet
        // $GLOBALS['STDERR'] = fopen(CWD_ROOT.'stderr.log','ab');
        ini_set('error_log',CWD_ROOT.'promptd_stderr.log');
    }


    public function run()
    {
        // the fopen goes here because that's the only thing that blocks?
        // https://stackoverflow.com/a/4361106
        $this->pipe_in = fopen($this->pipe_in_path,'r+');
#        stream_set_blocking($this->pipe_in,false);

        _stde('ready...',2);

        while( true )
        {
            // stream_set_blocking($this->pipe_in,true);
            // stream_set_read_buffer($this->pipe_in,0);
            // stream_set_write_buffer($this->pipe_in,0);

            // stream_set_blocking($this->socket,true);
            // stream_set_read_buffer($this->socket,0);
            // stream_set_write_buffer($this->socket,0);

            $read = [$this->pipe_in,$this->socket];
            $write = null;
            $except = null;
            $ready = stream_select($read,$write,$except,null);

            _stde($ready);

            if( $ready === FALSE )
            {
                _stde("select error");
                continue;
            }
            elseif( $ready > 0 )
            {
                foreach( $read as $stream )
                {
                    _stde($stream);
                    if( $stream === $this->socket )
                    {
                        $this->handle_socket();
                    }
                    else if( $stream === $this->pipe_in )
                    {
                        // will this handle long input?  is this always the whole prompt/request?  delims?
                        $this->prompt_buf = stream_get_contents($stream);
                        $this->handle_pipe();
                    }
                }
            }
        }
    }

    protected function handle_pipe()
    {
        file_put_contents(CWD_ROOT.'last_prompt.txt',$this->prompt_buf);
        $r = $this->pio->complete(blob:$this->prompt_buf,tries:3,temp:1.0);

        _stdo($r);
    }

    protected function handle_socket()
    {
        $client_socket = stream_socket_accept($this->socket,-1,$this->client_info);

        if( !$client_socket )
        {
            _stde('Error connecting client');
            return;
        }

        $client_pid = pcntl_fork();

        if( $client_pid === -1 )
        {
            _stde('Unable to fork for client.');
            exit(1);
        }
        // parent
        else if( $client_pid > 0 )
        {
            fclose($client_socket);
        }
        // child
        else
        {
            fclose($this->socket);
            $request = stream_get_contents($client_socket);
            fwrite($client_socket,"Your request was: $request");
            fclose($client_socket);
            exit(0);
        }
    }    
}


