<?php

class cnvyr
{
    public static function css( \asm\App $app )
    {
        $F = 'css_'.str_replace('.css','',\asm\Request::Path(-1));

        if( isset($app->html->$F) )
        {
            \asm\HTTP::ContentType('css','utf-8');
            $app->html->ReMap('Base',$F);
        }
        else
        {
            \asm\HTTP::_404();
            exit;
        }
    }

    public static function js(\asm\App $app )
    {
        $F = 'js_'.str_replace('.js','',\asm\Request::Path(-1));

        if( isset($app->html->$F) )
        {
            \asm\HTTP::ContentType('js','utf-8');
            $app->html->ReMap('Base',$F);
        }
        else
        {
            \asm\HTTP::_404();
            exit;
        }
    }

    public static function img( \asm\App $app )
    {
        ini_set('zlib.output_compression',FALSE);

        $MP = $app->Request['MatchPath'];

        if( empty($MP['Segments'][1]) )
        {
            \asm\HTTP::_404();
            exit;
        }

        $Token = $app->Config['Hostname'];
        $Filename = $MP['Segments'][1];
        $Config = array('Origin'=>$app->AppRoot.'/media/','NoLocalCache'=>TRUE);

        $cnvyr = new \asm\cnvyr('/tmp/cnvyr-cache/');
        $cnvyr->Srv(rawurldecode($Filename),$Token,$Config);
    }
}

