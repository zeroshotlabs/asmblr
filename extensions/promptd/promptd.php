<?php declare(strict_types=1);
/**
 * @file promptd.inc CLI GPT with local socket.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\extensions\promptd;



// @todo need a daemond or _d or so base class with forking, 
//configurable logs/etc
class promptd extends _d
{
    protected $prompt_buf;
    protected $response_buf;
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

