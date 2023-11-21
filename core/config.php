<?php declare(strict_types=1);
/**
 * @file config.php INI/JSON based configuration.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License v3.0 or later.
 * @copyright See COPYRIGHT.txt.
 */
namespace asm;


use asm\types\dao,asm\types\path;
use asm\_e\e500,asm\types\toke;
use function asm\sys\_stde;


// @todo enable local config overrides

/**
 * Manages the app's configuration via .ini files.
 * 
 * Normally this is a hidden file .settings.ini in the APP_ROOT.
 * 
 * @note This is a singleton.
 * @todo Put in an interface.
 */
class config
{
    public static $instance = null;
    public static $default_settings_path = APP_ROOT.'.settings.ini';

    /**
     * true only if status is 'live' or via custom method.  Used in $app etc.
     */
    public bool $is_production;

    /**
     * General app configuration settings.
     */
    public dao $app;

    /**
     * Endpoint definitions.
     */
    public array $endpoints;
    public array $endpoints_url_map = [];
    public string $endpoints_url_blob = '';

    /**
     * Template definitions.
     */
    public array $templates;

    /**
     * Singleton initializer.
     * 
     * The cache here is serialization of the object to disk.
     * 
     * @param string $cache Absolute to path to cache the configuration.  Must be writeable by php-fpm (ie. check /tmp OS config).
     * @note Across this class use of creds is deprecated in favor of secrets.ini - perhaps other things, too.
     * @todo use in-memory caching to avoid serialization and add lock/race
     */
    public static function init( string $settings_path = null,string $cache = null ): config
    {
        if( self::$instance )
            return self::$instance;

        if( is_file($cache??'') )
        {
            return unserialize(file_get_contents($cache));
        }
        else
        {
            self::$instance = new self($settings_path??self::$default_settings_path,$cache);

            if( !empty($cache) )
                file_put_contents($cache,serialize(self::$instance));

            return self::$instance;
        }
    }

    /**
     * Instantiate an object.
     * 
     * This is a singleton.
     */
    private function __construct( string $settings_path,string $cache = null )
    {
        if( !is_readable($settings_path))
            throw new e500("Settings file not found: $settings_path");
        
        try {
            $parsed_ini = parse_ini_file($settings_path,true,\INI_SCANNER_TYPED);
            $this->_build_config($parsed_ini);
        }
        catch( \Throwable $e )
        {
            throw new e500("Settings file '$settings_path' is not valid: ".$e->getMessage());
        }

        $this->is_production = ($this->app->status === 'live');

        // @todo future memory persistance (or in init)
        if( $cache )
        {
            file_put_contents($cache,serialize($this));
        }
    }

//     public function offsetGet( $key ): mixed
//     {
// //        var_dump(parent::offsetGet($key));
//         return parent::offsetGet($key);
// //        return new dao(parent::offsetGet(strtolower($key)));
//     }

//     public function __isset( $key ): bool
//     {
//         return parent::offsetExists($key);
//     }

    public function _build_config( array $parsed_ini ): bool
    {
        $conf = ['app'=>[],'endpoints'=>[],'endpoints_url_map'=>[],'endpoints_url_blob'=>'','templates'=>[]];

        $ptokens = function( $v ) {
            $t = [];
            foreach( $v as $i => $j )
                $t[$i] = class_exists($i)?$i:toke::parse_tokens((array)$j);
            return $t;
        };

        $ppath = function( $v ) {
            if( empty($v) )
                return '';
            else if( $v === '//' )
                return '//';
            else
                return ((string)path::url($v));
        };

        foreach( $parsed_ini as $k => $v )
        {
            if( $k === 'app' )
            {
                $conf['app'] = new dao($v);
            }
            else if( strpos($k,'endpoint_') === 0 )
            {
                $endpoint = ['name'=>substr($k,9),
                             'path'=>$ppath(trim($v['path']??'')),      // empty path means CLI
                           'greedy'=>((($v['path']??'') === '//') || (strpos(($v['path']??''),'//') > 0)),
                           'status'=>$v['status']??'',
                             'exec'=>self::_parse_exec($v['exec']??''),
                           'tokens'=>$ptokens($v['tokens']??[])];

                if( $endpoint['path'] && isset($conf['endpoints_url_map'][$endpoint['path']]) )
                    throw new e500("Duplicate endpoint path: {$endpoint['path']}");

                if( isset($conf['endpoints'][$endpoint['name']]) )
                    _stde("Overwriting duplicate endpoint name: {$endpoint['name']}");

                $conf['endpoints'][$endpoint['name']] = $endpoint;

                $conf['endpoints_url_map'][empty($endpoint['path'])?"_{$endpoint['name']}":$endpoint['path']] = &$conf['endpoints'][$endpoint['name']];
            }
            else if( strpos($k,'template_') === 0 )
            {
                $endpoint = ['name'=>substr($k,9),
                             'exec'=>self::_parse_exec($v['exec']??''),
                           'tokens'=>$ptokens($v['tokens']??[])];
 
                $conf['templates'][substr($k,9)] = new dao($v);
            }
        }

        // fast URL lookup string; line numbers correspond directly to $this->endpoints index.
        // used primarily for is_fes()
        $conf['endpoints_url_blob'] = implode(PHP_EOL,array_map(fn($v1,$v2): string => "$v1,$v2",array_keys($conf['endpoints_url_map']),array_keys($conf['endpoints'])));

        foreach( $conf as $k => $v )
            $this->{$k} = $v;

        return true;
    }

    /**
     * Normalizes a function/exec string defined for an endpoint or template.
     * 
     * The config string is converted to a class or class and method:
     *   class          [class] 
     *   class.method   [class,method]
     * 
     * @note first class callables didn't work out.
     */
    public static function _parse_exec( string $exec ): array 
    {
        if( trim($exec) === '' )
            return [];

        $exec1 = explode('.',$exec);

        if( count($exec1) === 1 )
        {
            return [$exec1[0]];
        }
        else if( count($exec1) === 2 )
        {
            return $exec1;
        }
        else
        {
            _stde("Malformed callable: $exec");
            return [];
        }
    }
}


/**
 * App secrets accessor.
 * 
  * @note This can be used as config to an extent.
  */
// function secrets( $reread = false,string $file = APP_ROOT.'/.settings.ini' ): ArrayObject
// {
//     static $parsed = null;

//     if( !$parsed || $reread )
//     {
//         $parsed = parse_ini_file($file,true,\INI_SCANNER_TYPED);
//         var_dump($parsed);
//         var_dump(json_decode($parsed['endpoint.global']['token']['page']));
//         var_dump(json_decode('{"title":"zero shot labs"}'));
//         exit;
//         if( $parsed )
//         {
//             $parsed = new \ArrayObject($parsed,\ArrayObject::ARRAY_AS_PROPS);
//             return $parsed;
//         }
//         else
//         {
//             _stde("secrets not found at $file");
//             return new \ArrayObject([]);
//         }
//     }
//     else
//         return $parsed;
// }
