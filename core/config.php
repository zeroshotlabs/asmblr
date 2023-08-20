<?php
/**
 * @file config.php CSV/Google Sheets based configuration.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Singleton credential manager.
 * 
 * Assumes a global $Credentials object is defined.
 * 
 * @see \asm\AppT::creds()
 */
function creds( $Group )
{
    static $Credentials = null;

    if( empty($Credentials) )
        $Credentials = new \Credentials;

    return ((object) $Credentials($Group));
}



// All labels are lowercase.
// it can't currently detect non-existant sheet tabs and the first is returned which causes a mess
// sheets tabs must be specified by name
class config
{
    use \asm\types\dynamic_kv;

    protected array $asmd;
    protected array $endpoints;
    protected array $templates;
    protected array $creds;
    protected array $meta;

//    protected array $obj_cache = ['asmd'=>null,'pages'=>null,'templates'=>null,'creds'=>null,'meta'=>null];


    /**
     * Template URL for Google Sheets GVIZ API.  8/23
     */ 
    protected $BaseURL = 'https://docs.google.com/spreadsheets/d/{sheetid}/gviz/tq?tqx=out:json&sheet={sheet}&headers=0';

    protected $_asmd_fields = ['status','sysop','baseurl','forcescheme','forcehost','forcepath'];

    /**
     * reserved keys for config rows
     *  - _asmd_fields
     *  - directive
     *  - page
     *  - template
     *  - name - signals
     */
    public function __construct( protected string $SheetID,protected array $SheetTabs )
    {
        $opts = [
            "http" => [
                "method" => "GET",
                "header" => "X-DataSource-Auth: true\r\n"
            ]
        ];
        
        $context = stream_context_create($opts);

        foreach( $SheetTabs as $ST )
        {
            $URL = str_replace(['{sheetid}','{sheet}'],[$this->SheetID,$ST],$this->BaseURL);

            $j = file_get_contents($URL,false,$context);

            // @note hardcode hack to get rid of random stuff from Google - then again, so is this whole thing
            $j = json_decode(substr($j,strpos($j,'{')),true);

            if( empty($j['table']['rows']) )
            {
                _stde("Empty config tab sheet '$ST' - no 'rows' - skipping");
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

                $col0_label = strtolower($row[0]);

                // use the first row to determine what type of sheet we have and send the rows
                // to the handler and skip to the next tab

                // this is the main config sheet asmd
                if( $i === 0 && in_array($col0_label,$this->_asmd_fields) )
                {
                    $this->_handle_asmd($j['table']['rows']);
                    continue 2;
                }
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
                    _stde("Unexpected thing 1: '$i'.");
            }

            $this->SheetTabs[$ST] = $j;
        }
    }

    public function __get( string $name )
    {


    }


    protected function _handle_asmd( array $rows ): void
    {
        $this->asmd = ['directives'=>[]];
        $last_name = $last_type = '';
        foreach( $rows as $row )
        {
            $row = $this->_sweeten($row);
            if( empty($row) )
            {
                _stde("Couldn't sweeten asmd row - bad structure: ".implode('|',$row));
                continue;
            }

            $col0_label = strtolower($row[0]);

            // add known keys and custom keys prefixed with _
            if( in_array($col0_label,$this->_asmd_fields) || ($col0_label[0] ?? '') === '_' )
            {
                $this->asmd[$col0_label] = $row[1];
            }
            else if( $col0_label === 'directive' )
            {
                $this->asmd['directives'][] = $this->_process_directive($row);
            }
            // @note both treated the same for now
            else if( $col0_label === 'page' || $col0_label === 'endpoint'  )
            {
                $last_name = $this->_process_endpoint($row);
                $last_type = 'endpoints';
            }
            else if( $col0_label === 'template' )
            {
                $last_name = $this->_process_template($row);
                $last_type = 'templates';
            }
            else if( $col0_label === '' && strtolower($row[1]) === 'directive' )
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
                _stde("Unexpected asmd config setting: $col0_label");
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
    protected function _handle_kv_sheet( array $rows,$name_label = 'name' ): array
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
    private function _sweeten( array $row ): array
    {
        return array_map(fn($v)=>!empty($v['v'])&&trim($v['v'])!=''?trim($v['v']):null,$row['c']);
    }

    private function _process_directive( array $row,int $offset = 0 ): array
    {
        return ['subject'=>$row[$offset+1],'key'=>$row[$offset+2],'value'=>$row[$offset+3]];
    }

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

        $this->endpoints[$lower_name] = ['name'=>$row[1],'url'=>$row[2],'status'=>$row[3],'function'=>$row[4],'directives'=>[]];
        return $lower_name;
    }
    
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

        $this->templates[$lower_name] = ['name'=>$row[1],'url'=>$row[2],'status'=>$row[3],'function'=>$row[4],'directives'=>[]];
        return $lower_name;
    }
}


