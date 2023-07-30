<?php
/**
 * @file prmptr.inc Prompt templating with Azure/OpenAI.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


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


class response extends DAO
{
    public $id,$type,$model,$response,$index,$logprobs,$finishReason,$result;

//    protected $ValidResponseTypes = ['json','xml','text'];
    protected $ValidationToken = '|!|';


    public function __construct( $response )
    {
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

    // automatically called upon getting a result
    public function IsValid()
    {
        if( strpos((string)$this->response,$this->ValidationToken) === 0 )
        {
            $this->response = trim(substr($this->response,strlen($this->ValidationToken)));
            return TRUE;
        }
        else
            return FALSE;
    }

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



class prmptr extends \asm\TemplateSet
{
    protected $client = NULL;

    protected $TemplateInit = '\asm\prompt::new';


    public function __construct( \asm\App $App )
    {
        parent::__construct($App);

        $this->IncludeExts[] = 'md';
    }

    public function ConnectAPI( $Vars )
    {
        // yeah!   probably should be passed in
        global $creds;

        $this->client = \OpenAI::factory()->withBaseUri($creds->AOAI['BaseURI'].'/'.$creds->AOAI['Model'])
                        ->withHttpHeader('api-key',$creds->AOAI['APIKey'])
                        ->withQueryParam('api-version',$creds->AOAI['APIVersion'])
                        ->make();
    }

    // // public function __construct( public string $Name,public string $Ask,public string $Func = '',public string $Model = '',
    // //                              public int $Tries = 1,public int $MaxTokens = 1024,public float $Temp = 1.0 )
    // public function __call( $Name,$Args )
    // {
    //     llog($Name);
    //     llog($this->Templates[$Name]);
    //     llog($Args);

    //     // prompt so render/capture/execute
    //     if( !empty($Args[0]) )
    //     {
    //         if( !empty($Args['tries']) )
    //             $tries = (int) $Args['tries'];
    //         else
    //             $tries = 1;

    //         return $this->completion($prompt,$tries);
    //     }


    //     // just a template render - buffer capture is done by the caller (the prompt itself)
    //     if( isset($this->Templates[$Name] ) )
    //     {
    //         parent::__call($Name,$Args);
    //     }
    // }
    
    public function chat( string $Name,array $roles = [],int $tries = 1,int $max_tokens = 1024,float $temp = 1.0,bool $rreturn = FALSE )
    {
        if( $this->client === NULL )
            $this->ConnectAPI(array());

        $chat = $this->render_return($Name);
        if( $rreturn )
            return $chat;
        var_dump($this->page);
        exit;

        if( $this->IsDebug() )
        {
            llog("starting chat $Name try of $tries");
            file_put_contents('/home/tmp/last_chat',$chat);
        }

        while( $tries-- )
        {
            // to use chatgpt specify the model:  'model' => 'text-davinci-003'
            // for azure, like this, the model is provided upon connect/endpoint
            $response = $this->client->chat()->create([
                'message' => $chat,
                'max_tokens' => $max_tokens,
                'temperature' => $temp
                ]);

            if( $this->IsDebug() )
                llog($response);

            if( count($response->choices) > 1 )
            {
                llog('Trying again - More than one answer for chat: '.substr($chat,0,32).'...');
                llog($response);
                llog('Trying again - More than one answer for chat: '.substr($chat,0,32).'...');
                continue;
            }

            if( $this->IsDebug() )
            {
                llog("response for $Name try of $tries");
                file_put_contents('/home/tmp/last_chat',\json_serialize($response),FILE_APPEND);
            }

            $result = new response($response);

            // requires the validation token to at least be present - the rest of validation (JSON/XML/etc) is left to the caller
//                return $result;

            llog("{$tries} tries for ".substr(trim($prompt),0,32));
        }

        return NULL;        
    }

    // // public string $Name,public string $Ask,public string $Func = '',public string $Model = '',
    // // public int $Tries = 1,public int $MaxTokens = 1024,public float $Temp = 1.0
    public function complete( string $Name,int $tries = 1,int $max_tokens = 1024,float $temp = 1.0,bool $rreturn = FALSE )
    {
        if( $this->client === NULL )
            $this->ConnectAPI(array());

        $prompt = $this->render_return($Name);
        if( $rreturn )
            return $prompt;

        if( $this->IsDebug() )
        {
            llog("starting completion $Name try of $tries");
            file_put_contents('/home/tmp/last_prompt',$prompt);
        }

        while( $tries-- )
        {
            // to use chatgpt specify the model:  'model' => 'text-davinci-003'
            // for azure, like this, the model is provided upon connect
            $response = $this->client->completions()->create([
                'prompt' => $prompt,
                'max_tokens' => $max_tokens,
                'temperature' => $temp
                ]);

            if( $this->IsDebug() )
                llog($response);

            if( count($response->choices) > 1 )
            {
                llog('Trying again - More than one answer for prompt: '.substr(trim($prompt),0,32).'...');
                llog($response);
                llog('Trying again - More than one answer for prompt: '.substr(trim($prompt),0,32).'...');
                continue;
            }
    
            $result = new response($response);

            // requires the validation token to at least be present - the rest of validation (JSON/XML/etc) is left to the caller
            if( $result->IsValid() )
                return $result;

            llog("{$tries} tries for ".substr(trim($prompt),0,32));
        }

        return NULL;
    }
}

