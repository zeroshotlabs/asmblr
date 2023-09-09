<?php
/**
 * @file lib.inc promptd common.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\extensions\promptd;


use \asm\types\dao as dao;
use function \asm\sys\_stde as _stde;


class prompt extends dao
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
class response extends dao implements \Stringable
{
    public $id,$type,$created,$model,$response,$index,$logprobs,$finish_reason,$result;

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
                _stde('NULL JSON result');
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
