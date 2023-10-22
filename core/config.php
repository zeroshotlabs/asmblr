<?php declare(strict_types=1);
/**
 * @file config.php CSV/Google Sheets based configuration.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;

use ArrayObject;
use asm\types\dao;
use asm\_e\e500;
use function asm\sys\_stde;


// @todo avoid having to map endpoints to files by using dir structure
// by default

// All labels are lowercase.
// it can't currently detect non-existant sheet tabs and the first is returned which causes a mess
// sheets tabs must be specified by name
// handles secrets too
// @todo enable local config overrides
// @todo  caching?!?!?!?
class config extends \asm\types\dao
{
    public static $instance = null;

    /**
     * true only if Status is 'live' or via domain detection.  Used in $app
     */
    public bool $is_production;

    /**
     * 
     */
    public array $app;
    public array $endpoints;
    public array $endpoints_url_map = [];
    public string $endpoints_url_blob = '';
    public array $templates;
    public array $creds;
    public array $meta;

    /**
     * Template URL for Google Sheets GVIZ API.  8/23
     */ 
    public $google_skel_url = 'https://docs.google.com/spreadsheets/d/{sheetid}/gviz/tq?tqx=out:json&sheet={sheet}&headers=0';

    public $app_fields = ['status','sysop'];


    /**
     * Singleton initializer, using a Google Sheet as the backend.
     * 
     * The cache here is serialization of the object to disk.  While fast, future extensions may provide in-memory
     * caching without serialization.
     * 
     * @param string $sheet_id Google Sheet's document ID from URL such as "... speadsheets/d/<sheet_id>/edit ..."
     * @param array $sheet_tabs The names of sheet tabs to use; defaults to app.
     *                          Other tabs will be processed as generic key/value pairs.
     * @param string $cache Absolute to path to cache the configuration.  Must be writeable by php-fpm (ie. check /tmp OS config).
     * @note Across this class use of creds is deprecated in favor of secrets.ini - perhaps other things, too.
     */
    public static function init( string $sheet_id,array $sheet_tabs = ['app'],string $cache = '' )
    {
        if( self::$instance )
            return self::$instance;

        if( is_file($cache) )
        {
            return unserialize(file_get_contents($cache));
        }
        else
        {
            self::$instance = new self($sheet_id,$sheet_tabs,$cache);

            if( !empty($cache) )
                file_put_contents($cache,serialize(self::$instance));

            return self::$instance;
        }
    }


    /**
     * Instantiate an object.
     * 
     * This is a singleton.
     * 
     * Reserved keys for config rows - checked on the first or second column.
     *  - app_fields
     *  - directive
     *  - page
     *  - template
     *  - name - signals (TODO)
     */
    public function __construct( public string $sheet_id,public array $sheet_tabs,string $cache = '' )
    {
        $opts = [
            "http" => [
                "method" => "GET",
                "header" => "X-DataSource-Auth: true\r\n"
            ]
        ];
        
        $context = stream_context_create($opts);

        foreach( $this->sheet_tabs as $tab )
        {
            $URL = str_replace(['{sheetid}','{sheet}'],[$this->sheet_id,$tab],$this->google_skel_url);

            $j = file_get_contents($URL,false,$context);

            // @note hardcode hack to get rid of random stuff from Google - then again, so is this whole thing
            $j = json_decode(substr($j,strpos($j,'{')),true);

            if( empty($j['table']['rows']) )
            {
                _stde("Empty config tab sheet '$tab' - no 'rows' - skipping");
                continue;
            }

            foreach( $j['table']['rows'] as $i => $row )
            {
                if( empty($row['c']) || !is_array($row['c']) )
                {
                    _stde("Invalid config row - no 'c' - skipping: ".implode('|',$row));
                    continue;
                }

                $row = $this->_sweeten($row);
                if( empty($row) )
                {
                    _stde("Couldn't sweeten config row - bad structure: ".implode('|',$row));
                    continue;
                }

                if( trim(implode('',$row)) === '' )
                    continue;

                $col0_label = strtolower($row[0]);

                // use the first row to determine what type of sheet we have and send the rows
                // to the handler and skip to the next tab

                // this is the main config sheet of the app
                if( $i === 0 && in_array($col0_label,$this->app_fields) )
                {
                    $this->_handle_app($j['table']['rows']);
                    continue 2;
                }
                // @note deprecated in favor of secrets.ini
                else if( $i === 0 && $col0_label === 'creds')
                {
                    $t = $this->_handle_kv_sheet($j['table']['rows'],'creds');
                    $this->creds[$t[0]] = $t[1];
                    continue 2;
                }
                // other sheets are stored as key/value pairs
                // cell 0,0 must be the key 'name' followed by the name.
                else if( $i === 0  )
                {
                    $t = $this->_handle_kv_sheet($j['table']['rows']);
                    $this->meta[$t[0]] = $t[1];
                    continue 2;
                }
                else
                    _stde("Unexpected thing 1: '$i'");
            }

            $this->sheet_tabs[$tab] = $j;
        }

        // fast URL lookup string; line numbers correspond directly to $this->endpoints index.
        $this->endpoints_url_blob = implode(PHP_EOL,array_keys($this->endpoints_url_map));

        // @todo future memory persistance
        if( $cache )
        {
            file_put_contents($cache,serialize($this));
        }
    }

    /**
     * Read a config value by populated section:
     *   - app
     *   - endpoints
     *   - endpoints_url_map
     *   - templates
     *   - creds
     *   - meta
     * 
     * @return dao object of configuration directives, keyed on directive name.
     * @todo probably should cache objects.
     */
    public function __get( $key ): dao
    {
        return new dao($this->{strtolower($key)} ?? []);
    }

    public function __isset( $key ): bool
    {
        return isset($this->{strtolower($key)});
    }

    public function _handle_app( array $rows ): void
    {
        $this->app = ['directives'=>[]];
        $last_name = $last_type = '';
        foreach( $rows as $row )
        {
            $row = $this->_sweeten($row);
            if( empty($row) )
            {
                _stde("Couldn't sweeten app row - bad structure: ".implode('|',$row));
                continue;
            }

            $col0_label = strtolower($row[0]??'');

            // add known keys and custom keys prefixed with _
            if( in_array($col0_label,$this->app_fields) || ($col0_label[0] ?? '') === '_' )
            {
                if( $col0_label === 'base_url' && !empty($row[1]) && strpos($row[1],'://') === false )
                    throw new e500("Invalid base_url - must contain '://': {$row[1]}");

                if( $col0_label === 'status' )
                    $this->is_production = $row[1]==='live'?true:false;

                $this->app[$col0_label] = $row[1];
            }
            // @note might be done differently
            else if( $col0_label === 'directive' )
            {
                $this->app['directives'][] = $this->_process_directive($row);
            }
            else if( $col0_label === 'endpoint'  )
            {
                $last_name = $this->_process_endpoint($row);
                $last_type = 'endpoints';
            }
            else if( $col0_label === 'template' )
            {
                $last_name = $this->_process_template($row);
                $last_type = 'templates';
            }
            else if( $col0_label === '' && strtolower($row[1]??'') === 'directive' )
            {
                $d = $this->_process_directive($row,1);

                if( empty($last_name) )
                    _stde("Child Directive without a parent name - skipping: ".implode('|',$row));
                else
                    $this->{$last_type}[$last_name]['directives'][] = $d;
            }
            // @note commented a whole row
            else if( substr($col0_label,0,2) === '//' )
            {
                continue;
            }
            else
            {
                _stde("Unexpected app config setting: $col0_label");
            }
        }
    }


    /**
     * Loose key/value pairs, though a Name is required.
     * 
     * If values have a '{' followed later by a '}', they are treated as JSON and decoded.
     * 
     * This is used by most sheet tabs
     * 
     * @return array A tuple of Name and the key/value pairs
     */
    public function _handle_kv_sheet( array $rows,$name_label = 'name' ): array
    {
        $KVs = [];

        $Name = 'UNKNOWN';
        foreach( $rows as $row )
        {
            $row = $this->_sweeten($row);
            if( empty($row) )
            {
                _stde("Couldn't sweeten KV row in '$Name' - bad structure: ".implode('|',$row));
                continue;
            }

            if( strtolower($row[0]) === $name_label )
            {
                $Name = $row[1];
                if( empty($Name) )
                {
                    _stde("Empty KV sheet name - no op");
                    return [];
                }
            }
            else
            {
                // if a { comes before a }, assume JSON
                if( strpos($row[1],'{') < strpos($row[1],'}') )
                    $KVs[trim($row[0])] = json_decode($row[1],true);
                else
                    $KVs[trim($row[0])] = trim($row[1]);
            }
        }

        return [$Name,$KVs];
    }

    /**
     * Normalize a row from a Google sheet tab;
     * 
     * Assumes a certain hacky structure - if something breaks,
     * this is probably it or below.
     * 
     * @param array Numeric row array from Google sheets' ['c'] format (2023-08).
     * @param array Empty array if any empty row (nothing but null or empty strings)
     */
    public function _sweeten( array $row ): array
    {
        return array_map(fn($v)=>!empty($v['v'])&&trim($v['v'])!=''?trim($v['v']):'',$row['c']);
    }

    public function _process_directive( array $row,int $offset = 0 ): array
    {
        return ['subject'=>$row[$offset+1],'key'=>$row[$offset+2],'value'=>$row[$offset+3]];
    }

    /**
     * @note lowercases the name and URL; should use path class.
     * @note this contains the definition of the endpoint array, which should be centralized.
     * 
     * @note An endpoint is an array like:
     *    ['name'=>$row[1],'url'=>'','greedy'=>(bool)),'status'=>'',
     *     'exec'=>'something::somewhere'','directives'=>[]]
     */
    private function _process_endpoint( array $row ): string
    {
        if( empty($row[1]) )
        {
            _stde("Empty endpoint name - skipping: ".implode('|',$row));
            return null;
        }

        $lower_name = strtolower($row[1]);
        if( !empty($this->endpoints[$lower_name]) )
            _stde("Duplicate endpoint name: {$lower_name} - overwriting!");

        $lower_url = strtolower($row[2]);

        // @note these are CLI endpoints.  They won't be in the URL map and they
        // are never greedy (they are called by name).
        if( empty($lower_url) )
        {
            $this->endpoints[$lower_name] = ['name'=>$row[1],'url'=>'','greedy'=>false,'status'=>$row[3],'exec'=>$row[4],'directives'=>[]];
        }
        else
        {
            // @note greedy endpoint; won't include the super-root.
            if( strpos($lower_url,'//') > 0 )
                $this->endpoints[$lower_name] = ['name'=>$row[1],'url'=>$lower_url,'greedy'=>true,'status'=>$row[3],'exec'=>$row[4],'directives'=>[]];  
            else
                $this->endpoints[$lower_name] = ['name'=>$row[1],'url'=>$lower_url,'greedy'=>false,'status'=>$row[3],'exec'=>$row[4],'directives'=>[]];  

            if( !empty($this->endpoints_url_map[$lower_url]) )
                _stde("Duplicate endpoint URL: {$lower_url} - overwriting!");
    
            $this->endpoints_url_map[$lower_url] = $this->endpoints[$lower_name];
        }

        return $lower_name;
    }
    
    /**
     * @todo review.
     */
    private function _process_template( array $row ): string
    {
        if( empty($row[1]) )
        {
            _stde("Empty template name - skipping: ".implode('|',$row));
            return null;
        }
        $lower_name = strtolower($row[1]);
        if( !empty($this->templates[$lower_name]) )
            _stde("Duplicate template name: {$lower_name} - overwriting!");

        $this->templates[$lower_name] = ['name'=>$row[1],'url'=>$row[2],'status'=>$row[3],'exec'=>$row[4],'directives'=>[]];
        return $lower_name;
    }
}


/**
 * App secrets accessor.
 * 
 * @note This can be used as config to an extent.
 */
function secrets( string $file = APP_ROOT.'/secrets.ini' ): ArrayObject
{
    static $parsed = null;

    if( !$parsed )
    {
        $parsed = parse_ini_file($file,true,\INI_SCANNER_TYPED);
        if( $parsed )
        {
            $parsed = new \ArrayObject($parsed,\ArrayObject::ARRAY_AS_PROPS);
            return $parsed;
        }
        else
        {
            _stde("secrets not found at $file");
            return new \ArrayObject([]);
        }
    }
    else
        return $parsed;
}
