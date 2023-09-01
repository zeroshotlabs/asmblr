<?php
/**
 * @file promptdio.inc Prompt templating with Azure/OpenAI.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\promptd;


/**
 * Manages prompts as text templates and executes them against OpenAI/ChatGPT
 */
class promptdio extends \asm\TemplateSet
{
    protected $client = NULL;

    protected $TemplateInit = '\asm\prompt::new';


    public function __construct( \asm\App $App,$creds,$connect = TRUE )
    {
        $this->IncludeExts += ['md','php','html'];

        parent::__construct($App);

        if( $connect === TRUE )
            $this->ConnectAPI($creds);
    }

    public function ConnectAPI( object $creds ): void
    {
        try
        {
            $this->client = \OpenAI::factory()->withBaseUri($creds->BaseURI.'/'.$creds->Model)
                                    ->withHttpHeader('api-key',$creds->APIKey)
                                    ->withQueryParam('api-version',$creds->APIVersion)
                                    ->make();
        }
        catch( \Exception $e )
        {
            throw new \Exception('Unable to connect to OpenAI/ChatGPT');
        }
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
    
    public function chat( string $Name = NULL,string $blob = NULL,array $roles = [],int $tries = 1,int $max_tokens = 1024,float $temp = 1.0,bool $rreturn = FALSE )
    {
        if( $this->client === NULL )
            throw new \Exception("No GPT connected $Name");

        if( !$Name && !$blob )
            throw new \Exception('prmptdio::chat() requires a prompt $Name or $Blob');

        if( $Name )
            $chat = $this->$Name(RReturn:TRUE);
        else
        {
            $chat = $blob;
            $Name = 'blob('.strlen($blob).')';
        }

        if( $rreturn )
            return $chat;

        if( $this->IsDebug() )
        {
            llog("starting chat $Name try of $tries");
            file_put_contents('/tmp/last_chat',$chat);
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
                file_put_contents('/tmp/last_chat',json_encode($response),FILE_APPEND);
            }

            // requires the validation token to at least be present, if set - the rest of validation (JSON/XML/etc) is left to the caller
            return new response($response);
        }

        return NULL;        
    }

    // // public string $Name,public string $Ask,public string $Func = '',public string $Model = '',
    // // public int $Tries = 1,public int $MaxTokens = 1024,public float $Temp = 1.0
    public function complete( string $Name = NULL,string $blob = NULL,int $tries = 1,int $max_tokens = 1024,float $temp = 1.0,bool $rreturn = FALSE )
    {
        if( $this->client === NULL )
            throw new \Exception("No GPT connected $Name");

        if( !$Name && !$blob )
            throw new \Exception('prmptdio::complete() requires a prompt $Name or $Blob');

        if( $Name )
            $prompt = $this->$Name(RReturn:TRUE);
        else
        {
            $prompt = $blob;
            $Name = 'blob('.strlen($prompt).')';
        }

        if( $rreturn )
            return $prompt;

        if( $this->IsDebug() )
        {
            llog("starting completion $Name try of $tries");
            file_put_contents('/tmp/last_prompt',$prompt);
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
