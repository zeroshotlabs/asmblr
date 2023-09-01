<?php
/**
 * @file lib.inc promptd common.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\promptd;

use \asm\DAO as DAO;
use \asm\Exception as Exception;


class prompt extends DAO
{
    // if we want to use chatgpt have to specify the model:  'model' => 'text-davinci-003',
    // functions not supported currently (asmblr template functions)
    // protected static $Skel = ['Name'=>'','Function'=>array(),'Model'=>'','Ask'=>'','Tries'=>1,'MaxTokens'=>1024,'Temp'=>1.0];

    public function __construct( public string $Name,public string $Path,public string $Ask,public array $Func = [],public string $Model = '',
                                 public int $Tries = 1,public int $MaxTokens = 1024,public float $Temp = 1.0 )
    {
    }

    // ( $Name,$Path,$Function = array(),$Body = '' )
    //($Prefix.$P['Segments'][1],$K,$F,$Buf);
    public static function new(): static
    {
        $args = func_get_args();
        return new static($args[0],$args[1],file_get_contents($args[1]));
    }
}

// currently supports only a single response
class response extends DAO implements \Stringable
{
    public $id,$type,$model,$response,$index,$logprobs,$finishReason,$result;

    // popular: |\.:/|
    protected $validation_token = NULL;


    public function __construct( $response,$validation_token = NULL )
    {
        $this->validation_token = $validation_token;
        $this->id = $response['id'];
        $this->type = $response['object'];
        $this->created = strtotime($response['created']);
        $this->model = $response['model'];

        if( isset($response['choices'][0]) )
        {
            $this->response = trim($response['choices'][0]['text']);
            $this->index = $response['choices'][0]['index'];
            $this->logprobs = $response['choices'][0]['logprobs'];
            $this->finish_reason = $response['choices'][0]['finish_reason'];
        }
    }

    public function __toString()
    {
        return $this->response;
    }

    // automatically called upon getting a result
    public function IsValid()
    {
        if( !$this->validation_token || (strpos((string)$this->response,$this->validation_token) === 0) )
        {
            if( $this->validation_token )
                $this->response = trim(substr($this->response,strlen($this->validation_token)));

            return TRUE;
        }
        else
            return FALSE;
    }

    // automatically decodes if it detects json
    public function IsJSON()
    {
        if( $this->response[0] === '[' || $this->response[0] === '{' )
        {
            $this->result = json_decode(mb_convert_encoding($this->response,"UTF-8"));

            if( $this->result !== NULL )
                return TRUE;
            else
            {
                llog('NULL JSON result');
                return FALSE;
            }
        }
        else
            return FALSE;
    }

    public function IsXML( $validation = null )
    {
        $xml = simplexml_load_string((string)$this->response,NULL,LIBXML_DTDVALID);

        if( $xml )
        {
            $this->result = $xml;
            return TRUE;
        }
        else
            return FALSE;
    }
}

/** not really used buy might come in handy **/
class fs_utils
{
    public $file;
    public $files;

    public function __construct()
    {
        $this->file = [];
        $this->files = [];
    }

    // populates $this->file
    function open_file( $path,$mask = '' )
    {
        if( !is_file($path) )
            throw new Exception("File '$path' not valid.");

        $path = realpath($path);

        $mask_len = strlen($mask);
        $url = substr($path,0,$mask_len);

        $ct = \asm\HTTP::Filename2ContentType($path);

        $this->file[$url] = ['content_type'=>$ct,
                                      'url'=>$url,
                                  'content'=>file_get_contents($path)];

        $char_cnt = strlen($this->file[$url]['content']);

        _stdo("\n{$char_cnt} - $path");
    }

    // populates $this->files
    function open_dir( $path )
    {
        $this->files = [];

        if( !is_dir($path) )
            throw new Exception("Directory '$path' not valid.");

        $path = realpath($path);

        $path_len = strlen($path);


        try {

            // Get all HTML type files recursively from the provided directory
            // quick change to add all files to a single string
            $file_i = new \RecursiveIteratorIterator(new \RecursiveDirectoryIterator($path,\FilesystemIterator::KEY_AS_PATHNAME & \FilesystemIterator::SKIP_DOTS & \RecursiveIteratorIterator::CATCH_GET_CHILD));
            $file_cnt = $char_cnt = 0;

            foreach( $file_i as $pathname => $file_j )
            {
                if( $file_j->isFile() && in_array(strtolower(pathinfo($file_j->getFilename(),PATHINFO_EXTENSION)),['html','txt','php','tpl','htm','css','js'] ) )
                {
                    $ct = \asm\HTTP::Filename2ContentType($pathname);
                    $url = substr($pathname,0,$path_len);
                    $this->files[$url] = ['content_type'=>$ct,
                                    'url'=>$url,
                                    'content'=>file_get_contents($file_j->getPathname())];
                    $file_cnt++;
                    $char_cnt += strlen($this->files[$url]['content']);
                    _stdo("{$char_cnt} - $pathname - $url");
                }
            }
        } catch( Exception $e ) {
            _stdo("permission denied for $pathname");
        }

        return [$file_cnt,$char_cnt];
    }
}


