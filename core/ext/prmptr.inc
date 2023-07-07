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


enum ValidResponseTypes
{
    case json;
    case xml;
}

/***
 * @todo considering advancements in objects, we should have a data-object that isn't based on an array, like a struct is.
 * here is a quick base data obj - see the DAO trait in Base
 */
class prompt implements \ArrayAccess
{
    use \asm\DAO;

    // if we want to use chatgpt have to specify the model:  'model' => 'text-davinci-003',
    // functions not supported currently
    // protected static $Skel = ['Name'=>'','Function'=>array(),'Model'=>'','Ask'=>'','Tries'=>1,'MaxTokens'=>1024,'Temp'=>1.0];

    public function __construct( public string $Name,public string $Path,public string $Ask,public array $Func = [],public string $Model = '',
                                 public int $Tries = 1,public int $MaxTokens = 1024,public float $Temp = 1.0 )
    {
    }

    // ( $Name,$Path,$Function = array(),$Body = '' )
    //($Prefix.$P['Segments'][1],$K,$F,$Buf);
    public static function new()
    {
        $args = func_get_args();
        return new self($args[0],$args[1],file_get_contents($args[1]));
    }
    
    public function IsValid( enum $ValidResponseTypes,$Token = NULL )
    {
        if( $Token === NULL )
            $Token = $this->ValidationToken;

//        if( strpos($))

    }
}

class response extends prompt
{
    protected string $ValidationToken = '|!|';

    public function __construct( $response )
    {
        llog($response);
    }
}



class prmptr extends \asm\TemplateSet
{
    protected $client = NULL;

    protected $TemplateInit = '\asm\prompt::new';


    public function __construct( \asm\App $App )
    {
        $this->App = $App;
        // @todo should change re other stuff
        $this->Templates = $App->Templates;

        $this->IncludeExts[] = 'md';
    }

    public function ConnectAPI( $Vars )
    {
        // yeah!
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
    

    // // public string $Name,public string $Ask,public string $Func = '',public string $Model = '',
    // // public int $Tries = 1,public int $MaxTokens = 1024,public float $Temp = 1.0
    public function complete( string $Name,int $tries = 1,int $max_tokens = 1024,float $temp = 1.0 )
    {
        if( $this->client === NULL )
            $this->ConnectAPI(array());


        ob_start();
        $this->__call($Name,[]);
        $prompt = ob_get_clean();

        if( $this->IsDebug() )
        {
            llog("starting $Name try of $tries");
            file_put_contents('/home/tmp/last_prompt',$prompt);
        }

        while( $tries-- )
        {
            // to use chatgpt specify the model:  'model' => 'text-davinci-003'
            // for azure, like this, the model is provided upon connect
            $result = $this->client->completions()->create([
                'prompt' => $prompt,
                'max_tokens' => $max_tokens,
                'temperature' => $temp
                ]);

            if( $this->IsDebug() )
                llog($result);

            if( count($result->choices) > 1 )
            {
                llog('Trying again - More than one answer for prompt: '.substr($prompt,0,32).'...');
                llog($result);
                llog('Trying again - More than one answer for prompt: '.substr($prompt,0,32).'...');
                continue;
            }
    
            // doesn't pay attention to result metrics yet; cheap check to detect JSON response
            # $r = ['id'=>$result->id,'ts'=>$result->created];
    
            if( !empty($result->choices[0]->text) )
            {
                $rstr = trim($result->choices[0]->text);
                //JSON :)
                if( $rstr[0] === '[' || $rstr === '{' )
                {
                    $r = json_decode($rstr);
                    if( $r !== NULL )
                        return $r;
                }
                else
                {
                    if( !empty($rstr) )
                        return $rstr;
                }
            }
            else
            {
                llog("Unexpected response: ",$result);
            }

            llog("{$tries} tries for ".substr($prompt,0,32));
        }

        return NULL;
    }

}

