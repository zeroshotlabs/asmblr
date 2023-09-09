<?php declare(strict_types=1);
/**
 * @file _d.inc CLI GPT with local socket.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;

abstract class _d
{
    protected $chroot = '/';
    protected $pid = null;

    protected $socket_path = CWD_ROOT.'_d.sock';
    protected $pipe_in_path = CWD_ROOT.'_d.pipe.in';

    protected $socket;
    protected $pipe_in;

    protected $client_info;

    protected $stdout_path;
    protected $stderr_path;



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
     * Daemonize the current process.
     * 
     * Double forks, closes stdin, stdout, stderr, and sets session id to fully detach.
     * 
     * stdout/stderr are reopened to log files.; stdin is not;
     */
    public function daemonize()
    {
        if( $this->stdout_path === null || $this->stderr_path === null )
            throw new \Exception('stdout_path and stderr_path must be set before daemonizing.');

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
        // @note however none of this is very reliable; PHP should do better
        fclose(STDIN);
        fclose(STDOUT);
        fclose(STDERR);
    
        // re-establish stdout - use _stdo/_stde from here on out
        $GLOBALS['STDOUT'] = fopen($this->stdout_path,'a');
    
        // @note messing with STDERR didn't work - probably some startup error thing
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
