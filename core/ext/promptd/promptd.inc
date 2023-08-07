<?php
/**
 * @file promptd.inc CLI GPT with local socket.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\promptd;

// @todo need a daemond or _d or so base class with forking, configurable logs/etc
class promptd
{
    protected $pid = null;

    protected $socket_path;
    protected $socket;

    protected $pipe_in_path;
    protected $pipe_in;

    protected $client_info;
    protected $prompt_buf;
    protected $response_buf;


    public function __construct( protected \asm\App $app,protected \asm\promptd\promptdio $pio )
    {
        // Daemonize doobly
        $this->pid = pcntl_fork();

        if( $this->pid === -1 )
            exit("Unable to daemonize fork");
        else if( $this->pid !== 0 )
            exit(0);
    
        $this->pid = pcntl_fork();

        if( $this->pid === -1 )
            exit("Unable to daemonize fork 2");
        else if( $this->pid !== 0 )
            exit(0);

        if( posix_setsid() === -1 )
            exit("Error: Unable to set session id.");

        chdir('/');

        $this->pipe_in_path = CWD_ROOT.'promptd.pipe_in';
        $this->socket_path = CWD_ROOT.'promptd.sock';

        // close std* and re-establish as $STDOUT, etc. PHP should do better?
        fclose(STDIN);
        fclose(STDOUT);
        fclose(STDERR);

        // re-establish stdout - use _stdo/_stde from here on out
        $GLOBALS['STDOUT'] = fopen(CWD_ROOT.'promptd_stdout.log','a');

        // @note messing with STDERR doesn't appear to work - probably some startup error thing
        // $GLOBALS['STDERR'] = fopen(CWD_ROOT.'stderr.log','ab');
        ini_set('error_log',CWD_ROOT.'promptd_stderr.log');

        // accept prompts via pipe
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

        // @todo sockets aren't done
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

    public function run()
    {
        _stde('ready...');

        while( true )
        {
            // the fopen goes here because that's the only thing that blocks?
            // https://stackoverflow.com/a/4361106
            $this->pipe_in = fopen($this->pipe_in_path,'r');

            stream_set_blocking($this->pipe_in,true);
            stream_set_read_buffer($this->pipe_in,0);

            $read = [$this->pipe_in,$this->socket];
            $write = null;
            $except = null;

            $ready = stream_select($read,$write,$except,null);

            if( $ready === FALSE )
            {
                _stde("select error");
                continue;
            }
            elseif( $ready > 0 )
            {
                foreach( $read as $stream )
                {
                    if( $stream === $this->socket )
                    {
                        _stde('socket TODO');
                        // $this->handle_socket_client();
                    }
                    else if( $stream === $this->pipe_in )
                    {
                        // will this handle long input?  is this always the whole prompt/request?  delims?
                        $this->prompt_buf = stream_get_contents($stream);
                        $this->handle_prompt();
                    }
                }
            }
        }
    }

    protected function handle_prompt()
    {
        file_put_contents(CWD_ROOT.'last_prompt.txt',$this->prompt_buf);
        $r = $this->pio->complete(blob:$this->prompt_buf,tries:3,temp:1.0);

        _stdo($r);
    }

    // @todo not implemented yet
    protected function handle_socket_client()
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
        else if( $client_pid > 0 )
        {
            fclose($client_socket);
        }
        else
        {
            fclose($this->socket);
            $request = stream_get_contents($client_socket);
            _stdo("Request: $request");
        }
    }
}

// class prmptsh
// {
//     private $process;
//     private $pipes;
//     private $stdout;

//     public function __construct( $command = "/bin/bash -l" )
//     {
//         $fds = [ 0 => ["pipe", "r"], // stdin
//                  1 => ["pipe", "w"], // stdout
//                  2 => ["file", "/tmp/err.log","w"]  // stderr
//                ];

//         $this->process = proc_open($command,$fds,$this->pipes);

//         if( is_resource($this->process) )
//         {
// #            stream_set_blocking($this->pipes[0], false);
//             stream_set_blocking($this->pipes[1], false);
// #            stream_set_blocking($this->pipes[2], false);

//             $this->stdout = '';
//         }
//         else
//             throw new Exception("Unable to open command '$command'");
//     }

//     public function run()
//     {
//         while( true )
//         {

//             // $input = trim(fgets(STDIN));
//             // var_dump($input);
//             // if( $input === 'q' )
//             //     break;

//             // passthru("/bin/bash -c $input\n");
//             // continue;
            
//             // fwrite($this->pipes[0],$input."\n");

//             $read = [$this->pipes[1],STDIN];
//             $write = null;
//             $except = null;

//             $ready = stream_select($read,$write,$except,NULL);

//             if( $ready === FALSE )
//             {
//                 _stdo("select error");
//                 continue;
//             }
//             elseif( $ready > 0 )
//             {
//                 foreach( $read as $stream )
//                 {
//                     if( $stream === STDIN )
//                     {
//                         $input = fgets(STDIN);
//                         if( $input === "q\n" )
//                             break 2;
//                         fwrite($this->pipes[0],$input);
//                     }
//                     elseif( $stream === $this->pipes[1] )
//                     {
//                         $output = stream_get_contents($this->pipes[1]);
//                         echo $output;
//                     }
//                 }
//             }


//             // // Check if the user typed 'q' to exit the shell
//             // if (feof(STDIN)) {
//             //     break;
//             // }

//             // $input = fgets(STDIN);
//             // if (trim($input) === 'q') {
//             //     break;
//             // }

//         }
// echo 'END';
//         // Close the process and streams
//         fclose($this->pipes[0]);
//         fclose($this->pipes[1]);
// #        fclose($this->pipes[2]);

//         proc_close($this->process);
//     }
// }



// // worked but not great (no terminal color/etc)
// // class prmptsh
// // {
// //     private $process;
// //     private $pipes;
// //     private $stdout;

// //     public function __construct( $command = "/bin/bash -l" )
// //     {
// //         $fds = [ 0 => ["pipe", "r"], // stdin
// //                  1 => ["pipe", "w"], // stdout
// //                  2 => ["file", "/tmp/err.log","w"]  // stderr
// //                ];

// //         $this->process = proc_open($command,$fds,$this->pipes);

// //         if( is_resource($this->process) )
// //         {
// // #            stream_set_blocking($this->pipes[0], false);
// //             stream_set_blocking($this->pipes[1], false);
// // #            stream_set_blocking($this->pipes[2], false);

// //             $this->stdout = '';
// //         }
// //         else
// //             throw new Exception("Unable to open command '$command'");
// //     }

// //     public function run()
// //     {
// //         while( true )
// //         {

// //             // $input = trim(fgets(STDIN));
// //             // var_dump($input);
// //             // if( $input === 'q' )
// //             //     break;

// //             // passthru("/bin/bash -c $input\n");
// //             // continue;
            
// //             // fwrite($this->pipes[0],$input."\n");

// //             $read = [$this->pipes[1],STDIN];
// //             $write = null;
// //             $except = null;

// //             $ready = stream_select($read,$write,$except,NULL);

// //             if( $ready === FALSE )
// //             {
// //                 _stdo("select error");
// //                 continue;
// //             }
// //             elseif( $ready > 0 )
// //             {
// //                 foreach( $read as $stream )
// //                 {
// //                     if( $stream === STDIN )
// //                     {
// //                         $input = fgets(STDIN);
// //                         if( $input === "q\n" )
// //                             break 2;
// //                         fwrite($this->pipes[0],$input);
// //                     }
// //                     elseif( $stream === $this->pipes[1] )
// //                     {
// //                         $output = stream_get_contents($this->pipes[1]);
// //                         echo $output;
// //                     }
// //                 }
// //             }


// //             // // Check if the user typed 'q' to exit the shell
// //             // if (feof(STDIN)) {
// //             //     break;
// //             // }

// //             // $input = fgets(STDIN);
// //             // if (trim($input) === 'q') {
// //             //     break;
// //             // }

// //         }
// // echo 'END';
// //         // Close the process and streams
// //         fclose($this->pipes[0]);
// //         fclose($this->pipes[1]);
// // #        fclose($this->pipes[2]);

// //         proc_close($this->process);
// //     }
// // }

