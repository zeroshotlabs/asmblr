<?php
/**
 * @file HTTP.php URL and HTTP related tools.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;

/**
 * Tools for manipulating a URL.
 *
 * The components of a URL are:
 *
 *  @li @c IsHTTPS: @c TRUE if the scheme is https.
 *  @li @c scheme: Typically @c http or @c https.
 *  @li @c username
 *  @li @c password
 *  @li @c hostname
 *  @li @c port
 *  @li @c path: A @c path Struct
 *  @li @c encoded: A @c URLEncoded Struct
 *  @li @c fragment or @c #
 *
 * @note This does not support full URL/URI pedantics and basically for HTTP.
 */
class URL implements \Stringable
{
    // protected static $Skel = array('IsHTTPS'=>FALSE,'scheme'=>'','username'=>'','password'=>'',
    // 							   'hostname'=>array(),'port'=>'','path'=>array(),'encoded'=>array(),'fragment'=>'');

    /**
     * TRUE if the scheme is https.
     */
    public readonly bool $IsHTTPS;

//    public readonly \asm\hostname $hostname;

    /**
     * Create a new URL object.
     * 
     * No validation is done amongst the arguments.
     * 
     * Use URL::str() instead.
     */
    public function __construct(

        /**
         * Typically http or https without '://'
         */
        public readonly string $scheme,

        public readonly string $username,

        public readonly string $password,

        public readonly \asm\types\hostname $hostname,

        public readonly int $port,

        public readonly \asm\types\path $path,

        public readonly \asm\types\encoded $encoded,

        public readonly string $fragment
    )
    {
        $this->IsHTTPS = ($scheme === 'https');
    }

    /**
     * Create a new URL DAO from a string.
     *
     * This uses parse_url() and follows it's semantics, whereby some URL strings will be indeterminate.
     * 
     * @param string $URLStr The URL string to parse.
     * @throws Exception Malformed URL '$URLStr' (parse_url()).
     * @return \asm\URL A URL.
     *
     * @note This uses parse_url() and defaults to https if not specified. It attempts
     * to detect and auto-correct malformed URLs but it's not perfect; filename.html is
     * parsed as domain.com for example.
     */
    public static function str( $URLStr ): \asm\URL
    {
        if( !is_string($URLStr) || !empty($URLStr = trim($URLStr)) )
        {
            _stde("URL::str() - URLStr is not a string (".gettype($URLStr).")");
            return null;
        }

        // perform some preprocess for domain vs path vs URL detection magic if there isn't a scheme
        if( strpos($URLStr,'://') === false )
        {
            $p_p = strpos($URLStr,'.');
            $p_s = strpos($URLStr,'/');

            // a period before a slash, or no slash - prepend https://
            // domain.com  domain.com/  domain.com/something.html
            if( $p_p < $p_s || $p_s === false )
            {
                $URLStr = "https://{$URLStr}";
            }
            // a slash before a period or slash at first character - treat as an absolute path only
            else if( $p_s < $p_p || $p_s === 0 )
            {
                $URLStr = '/'.$URLStr;
            }
        }

        if( ($T = @parse_url(trim($URLStr))) === false )
            throw new Exception("Malformed URL '$URLStr' (parse_url()).");

        $scheme = strtolower($T['scheme'] ?? 'https');
        $hostname = types\hostname::str($T['host'] ?? '');
        $port = $T['port'] ?? 0;

        $username = $T['user'] ?? '';
        $password = $T['pass'] ?? '';

        $path = types\path::url($T['path'] ?? '');

        $encoded = types\encoded::str($T['encoded'] ?? '');
        $fragment = $T['fragment'] ?? '';

        return new self($scheme,$username,$password,$hostname,$port,$path,$encoded,$fragment);
    }

    /**
     * Create a URL string depending on the URL parts present.
     *
     * @return string URL string.
     *
     * @note Logic exists to handle an empty hostname in which case scheme/port/username/password isn't included
     *       and thus a path-only "URL" is returned.
     */
    public function __toString(): string
    {
        $host = (string) $this->hostname;
        if( !empty($host) )
        {
            if( $this->port && $this->port !== 80 && $this->port !== 443 )
                $host .= ":{$this->port}";

            $auth = '';
            if( !empty($this->username) )
            {
                $auth = rawurlencode($this->username);
                if( !empty($this->password) )
                    $auth .= ':'.rawurlencode($this->password);
                $auth .= '@';
            }
            $host = "{$this->scheme}://{$auth}{$host}";
        }

        $path = $this->path->as_abs();
        $encoded = (string) $this->encoded;
        $frag = !empty($this->fragment) ? '#'.rawurlencode($this->fragment) : '';

        return "{$host}{$path}{$encoded}{$frag}";

        // // if a hostname is present, ensure a / for relative paths - otherwise use what path has
        // $Str .= (!empty($Str)&&empty($URL['path']['IsAbs'])?'/':'').path::ToString($URL['path'],'url');
        // $Str .= empty($URL['encoded'])?'':URLEncoded::ToString($URL['encoded']);
        // $Str .= $URL['fragment']===''?'':'#'.rawurlencode($URL['fragment']);
    }
}



    // /**  FROM encodedSTRING
    //  * Add or remove key/values in a URLEncoded Struct.
    //  *
    //  * $Needle is change string, an array of change strings, or an array
    //  * of associative array key/value pairs, defining the changes to make:
    //  *  - @c ?key=value&key1=value1: set the key/value pairs from a string.  setting empty will delete the key.
    //  *  - @c ?: remove all key/values
    //  *  - @c array('key'=>'value'): set the key/value from an array
    //  *  - @c array('key'=>NULL): remove the key/value, if it exists
    //  *  - @c array(): remove all key/values
    //  *
    //  * @param array $Needle An array of key/values as strings or arrays.
    //  * @param string $Needle A key=value change string.
    //  * @param array &$Haystack URLEncoded Struct.
    //  * @retval void
    //  */
    // public static function Set( $Needle,&$Haystack )
    // {
    //     foreach( (array) $Needle as $K => $V )
    //     {
    //         if( empty($V) || $V === '?' )
    //         {
    //             $Haystack = array();
    //             continue;
    //         }

    //         if( is_string($V) )
    //         {
    //             parse_str(ltrim($V,'?'),$V2);
    //         }
    //         else if( is_array($V) )
    //         {
    //             $V2 = $V;
    //         }

    //         foreach( $V2 as $I => $J )
    //         {
    //             if( isset($Haystack[$I]) && empty($J) )
    //                 static::Del($I,$Haystack);
    //             else
    //                 $Haystack[$I] = $J;
    //         }
    //     }
    // }

/// WAS URL

    
    // /**
    //  * Helper for creating a URL authority part from a username and password.
    //  *
    //  * @param string|NULL $username The username, an empty string, or NULL.
    //  * @param string|NULL $password The password, an empty string, or NULL.
    //  * @retval string URL authority string, which may be an empty string.
    //  *
    //  * @note A password without a username will return an empty string.
    //  * @note Explicitly setting a username in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function AuthorityToString( $username,$password )
    // {
    //     if( $username !== NULL && $username !== '' )
    //         return rawurlencode($username).($password!==''&&$password!==NULL?':'.rawurlencode($password).'@':'@');
    //     else
    //         return '';
    // }
    // /**
    //  * Change parts of a URL Struct.
    //  *
    //  * $Needle is a change string or array of strings defining the changes to make:
    //  *  - @c >segment: append a segment to the path.
    //  *  - @c <segment: prepend a segment to the path.
    //  *  - @c ?key=value: set a key/value in the encoded string.
    //  *  - @c ?key=: delete a key/value in the encoded string.
    //  *  - @c ?: delete entire encoded string.
    //  *  - @c \#fragment: set the URL fragment.
    //  *  - @c #: delete the URL fragment.
    //  *
    //  * $Needle may also combine segment, ?key=value and \#fragment change strings:
    //  *  - \c >new-segment?login=1&register=&redirect=1
    //  *
    //  * This would append a new path segment, set login and redirect encoded variables
    //  * to 1, and remove the register variable.
    //  *
    //  * The array syntax for URLEncoded::Set() is also supported.
    //  *
    //  * @param string $Needle A change to make.
    //  * @param array $Needle An array of changes to make.
    //  * @param array &$Haystack URL Struct.
    //  *
    //  * @todo This needs optimization and perhaps better support, including appending/prepending
    //  *       more than one path segment at a time (i.e. from LinkPage).
    //  */
    // public static function Set( $Needle,&$Haystack )
    // {
    //     foreach( (array) $Needle as $K => $V )
    //     {
    //         if( is_array($V) )
    //         {
    //             URLEncoded::Set($V,$Haystack['encoded']);
    //             continue;
    //         }
    //         else if( !is_int($K) )
    //         {
    //             URLEncoded::Set(array(array($K=>$V)),$Haystack['encoded']);
    //             continue;
    //         }

    //         $V2 = explode('#',$V);
    //         if( isset($V2[1]) )
    //             static::Setfragment($V2[1],$Haystack);

    //         $V2 = explode('?',$V2[0]);
    //         if( isset($V2[1]) )
    //             URLEncoded::Set($V2[1],$Haystack['encoded']);

    //         if( !empty($V2[0]) )
    //         {
    //             if( ($V2[0][0] === '>' || $V2[0][0] === '<') )
    //             {
    //                 path::Set($V2[0],$Haystack['path']);
    //                 $Haystack['path']['IsAbs'] = TRUE;
    //             }
    //             else
    //             {
    //                 trigger_error("URL::Set() Unrecognized change string '{$V}'");
    //             }
    //         }
    //     }
    // }

    // /**
    //  * Read the full hostname as a string.
    //  *
    //  * @param array $URL URL Struct.
    //  * @retval string The hostname string.
    //  */
    // public static function hostname( $URL )
    // {
    //     return hostname::ToString($URL['hostname']);
    // }

    // /**
    //  * Read the full path as a string.
    //  *
    //  * @param array $URL URL Struct.
    //  * @retval string The path string.
    //  */
    // public static function path( $URL )
    // {
    //     return path::ToString($URL['path'],'url');
    // }

    // /**
    //  * Create a new URL from individual parts.
    //  *
    //  * @param string $scheme Typically http or https (:// is trimmed).
    //  * @param string $hostname hostname.
    //  * @param string|array $path path string or path Struct.
    //  * @param string|array $encoded encoded string or URLEncoded Struct.
    //  * @param string|NULL $port Optional port.
    //  * @param string|NULL $username Optional username.
    //  * @param string|NULL $password Optional password.
    //  * @param string|NULL $fragment Optional fragment.
    //  * @retval array The URL.
    //  *
    //  * @note Explicitly setting a username/password in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function InitParts( $scheme,$hostname,$path,$encoded,$port = NULL,$username = NULL,$password = NULL,$fragment = NULL )
    // {
    //     $URL = static::$Skel;

    //     static::Setscheme($scheme,$URL);
    //     static::Sethostname($hostname,$URL);
    //     static::Setpath($path,$URL);
    //     static::Setencoded($encoded,$URL);

    //     if( $port !== NULL )
    //         static::Setport($port,$URL);
    //     if( $username !== NULL )
    //         static::Setusername($username,$URL);
    //     if( $password !== NULL )
    //         static::Setpassword($password,$URL);
    //     if( $fragment !== NULL )
    //         static::Setfragment($fragment,$URL);

    //     return $URL;
    // }

    // /**
    //  * Change the scheme of a URL.
    //  *
    //  * @param string $scheme A string.
    //  * @param array $URL A URL Struct.
    //  *
    //  * @note This will affect the URL's IsHTTPS value.
    //  */
    // public static function Setscheme( $scheme,&$URL )
    // {
    //     $URL['scheme'] = strtolower(trim(trim($scheme,'://')));
    //     $URL['IsHTTPS'] = $URL['scheme']==='https'?TRUE:FALSE;
    // }

    // /**
    //  * Change the hostname of a URL.
    //  *
    //  * @param string|array $hostname A string or hostname Struct to set.
    //  * @param array $URL A URL Struct.
    //  * @throws Exception hostname not a string or hostname Struct.
    //  */
    // public static function Sethostname( $hostname,&$URL )
    // {
    //     if( is_string($hostname) === TRUE )
    //         $URL['hostname'] = hostname::Init($hostname);
    //     else if( is_array($hostname) === TRUE )
    //         $URL['hostname'] = $hostname;
    //     else
    //         throw new Exception("hostname not a string or hostname Struct.");
    // }

    // /**
    //  * Change the path of a URL.
    //  *
    //  * @param string|path $path A string or path Struct to set.
    //  * @param array $URL A URL Struct.
    //  * @throws Exception path not a string or path Struct.
    //  */
    // public static function Setpath( $path,&$URL )
    // {
    //     if( is_string($path) === TRUE )
    //         $URL['path'] = path::Init($path);
    //     else if( isset($path['Segments']) === TRUE )
    //         $URL['path'] = $path;
    //     else if( $path !== NULL )
    //         throw new Exception('path not a string or path Struct.');
    // }

    // /**
    //  * Change the encoded of a URL.
    //  *
    //  * @param string|array $encoded A string or URLEncoded Struct to set.
    //  * @param array $URL A URL Struct.
    //  * @throws Exception encoded not a string or URLEncoded Struct.
    //  */
    // public static function Setencoded( $encoded,&$URL )
    // {
    //     if( is_string($encoded) === TRUE )
    //         $URL['encoded'] = URLEncoded::Init($encoded);
    //     else if( is_array($encoded) === TRUE )
    //         $URL['encoded'] = $encoded;
    //     else if( $encoded !== NULL )
    //         throw new Exception("encoded not a string or URLEncoded Struct.");
    // }

    // /**
    //  * Change the port of a URL.
    //  *
    //  * @param string|int $port A string or integer.
    //  * @param URL $URL A URL Struct.
    //  *
    //  * @note The port is always converted to a string.
    //  * @note If the port is 80 or 443, it is set as the empty string.
    //  */
    // public static function Setport( $port,&$URL )
    // {
    //     if( in_array((string)$port,array('','80','443')) === TRUE )
    //         $URL['port'] = '';
    //     else
    //         $URL['port'] = (string) $port;
    // }

    // /**
    //  * Change the username of a URL.
    //  *
    //  * @param string $username A string.
    //  * @param array $URL A URL Struct.
    //  *
    //  * @note Explicitly setting a username in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function Setusername( $username,&$URL )
    // {
    //     $URL['username'] = $username===NULL?$URL['username']:$username;
    // }

    // /**
    //  * Change the password of a URL.
    //  *
    //  * @param string $password A string.
    //  * @param array $URL A URL Struct.
    //  *
    //  * @note A username check is not done.
    //  * @note Explicitly setting a password in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function Setpassword( $password,&$URL )
    // {
    //     $URL['password'] = $password===NULL?$URL['password']:$password;
    // }

    // /**
    //  * Change or delete the fragment of a URL.
    //  *
    //  * @param string $fragment fragment or a single # to delete an existing one.
    //  * @param array $URL A URL Struct.
    //  */
    // public static function Setfragment( $fragment,&$URL )
    // {
    //     if( $fragment === '#' )
    //         $URL['fragment'] = '';
    //     else if( $fragment !== NULL )
    //         $URL['fragment'] = ltrim($fragment,'#');
    // }



    //   WAS hostname
    // /**
    //  * Create a string from the hostname array.
    //  *
    //  * @param array $hostname A hostname Struct.
    //  * @retval string The hostname string.
    //  */
    // public static function ToString( $hostname )
    // {
    //     return implode('.',array_reverse($hostname));
    // }

    // /**
    //  * Add a subdomain to the "beginning" of a hostname.
    //  *
    //  * This remaps to Append().
    //  */
    // public static function Prepend( $Element,&$Subject )
    // {
    //     return parent::Append(trim(trim($Element),'.'),$Subject);
    // }

    // /**
    //  * Add a subdomain to the "end" of a hostname.
    //  *
    //  * This remaps to Prepend().
    //  */
    // public static function Append( $Element,&$Subject )
    // {
    //     return parent::Prepend(trim(trim($Element),'.'),$Subject);
    // }

    // /**
    //  * Prepend or append a subdomain in a hostname.
    //  *
    //  * $Needle is a string defining the change to make:
    //  *  - \c <subdomain: prepend the subdomain.
    //  *  - \c >subdomain: append the subdomain.
    //  *
    //  * @param string $Needle Direction and subdomain to add.
    //  * @param array $Haystack hostname Struct.
    //  * @retval int The position the subdomain was added.
    //  * @retval NULL Operation not recognized.
    //  *
    //  * @todo This may be expanded slightly.
    //  */
    // public static function Set( $Needle,&$Haystack )
    // {
    //     $Needle = trim($Needle);

    //     if( $Needle[0] === '<' )
    //         return static::Prepend(ltrim($Needle,'<'),$Haystack);
    //     else if( $Needle[0] === '>' )
    //         return static::Append(ltrim($Needle,'>'),$Haystack);
    //     else
    //         return NULL;
    // }

    // /**
    //  * Search for and return the position of a sub-domain.
    //  *
    //  * @param string $Needle The sub-domain to search for.
    //  * @param array $hostname The hostname Struct to search.
    //  * @retval int The position of the sub-domain.
    //  * @retval FALSE The sub-domain was not found.
    //  *
    //  * @note The search is case-sensitive.
    //  */
    // public static function Search( $Needle,$hostname )
    // {
    //     return array_search($Needle,$hostname);
    // }

    // /**
    //  * Read one or more sub-domains from the top.
    //  *
    //  * In a hostname such as www.asmblr.org, "org" is the top most sub-domain.
    //  *
    //  * @param array $Haystack hostname Struct.
    //  * @param int $Limit Optional number of sub-domains to read, starting from 1.
    //  * @retval string The single top-most sub-domain.
    //  * @retval array The specified number of top-most sub-domains as a new hostname Struct.
    //  */
    // public static function Top( $Haystack,$Limit = 0 )
    // {
    //     if( $Limit > 0 )
    //         return array_slice($Haystack,0,$Limit);
    //     else
    //         return $Haystack[0];
    // }

    // /**
    //  * Read one or more sub-domains from the bottom.
    //  *
    //  * In a hostname such as www.asmblr.org, "www" is the bottom most sub-domain.
    //  *
    //  * @param array $Haystack hostname Struct.
    //  * @param int $Limit Optional number of sub-domains to read, starting from 1.
    //  * @retval string The single bottom-most sub-domain.
    //  * @retval array The specified number of bottom-most sub-domains as a new hostname Struct.
    //  */
    // public static function Bottom( $Haystack,$Limit = 0 )
    // {
    //     if( $Limit > 0 )
    //         return array_slice($Haystack,count($Haystack)-$Limit);
    //     else
    //         return $Haystack[count($Haystack)-1];
    // }


    // /**
    //  * Iterate through a hostname sub-domain by sub-domain, left to right or right to left.
    //  *
    //  * This creates an array containing increasingly more or less specific versions of
    //  * the same hostname.
    //  *
    //  * By default this returns an array in increasing hostname size, i.e. most general to
    //  * most specific.
    //  *
    //  * @param array $hostname The hostname Struct.
    //  * @param boolean $Inc Iterate in increasing hostname size, i.e., most general to most specific.
    //  * @retval array Ordered hostname sub-domains.
    //  *
    //  * @note Each hostname returned will have a leading period and no trailing period.
    //  */
    // public static function Order( $hostname,$Inc = TRUE )
    // {
    //     $P = array();
    //     foreach( $hostname as $K => $V )
    //         $P[] = ".{$V}".($K>0?$P[$K-1]:'');

    //     if( $Inc === TRUE )
    //         return $P;
    //     else
    //         return array_reverse($P);
    // }




    // /** WAS PATH
    //  * Merge $Src segments into $Dest segments.
    //  *
    //  * If $Src has the same segment at the same position as $Dest, it is
    //  * skipped.  Otherwise, $Src segments are appended to $Dest.
    //  *
    //  * @param path $Src The path to merge in.
    //  * @param path $Dest A reference to the base path to merge into.
    //  *
    //  * @note All comparisions are type strict (===).
    //  * @note This is a no-op if $Src IsRoot is TRUE.
    //  * @note $Dest will have same IsDir as $Src.
    //  * @note $Dest will become IsRoot FALSE if the merge occurs.
    //  */
    // public static function Merge( $Src,&$Dest )
    // {
    //     if( $Src['IsRoot'] === TRUE )
    //         return;

    //     if( $Dest['IsRoot'] === TRUE )
    //         $Dest['Segments'] = array();

    //     $Dest['IsDir'] = $Src['IsDir'];
    //     $Dest['IsRoot'] = FALSE;

    //     foreach( $Src['Segments'] as $K => $V )
    //     {
    //         if( isset($Dest['Segments'][$K]) === TRUE && $Dest['Segments'][$K] === $V )
    //             continue;
    //         else
    //             $Dest['Segments'][] = $V;
    //     }
    // }

    // /**
    //  * Remove matching segments that exist in $Mask from $Base.
    //  *
    //  * If $Mask contains the same segments in the same positions as $Base,
    //  * remove those matching segments from $Base.
    //  *
    //  * @param path $Mask The path that masks segments.
    //  * @param path $Base The path that will have segments removed.
    //  * @retval void
    //  *
    //  * @note This is implemented using array_diff() and array_values().
    //  * @note This is a no-op if either $Mask or $Base is IsRoot TRUE.
    //  * @note This may cause $Base to become IsRoot and IsDir TRUE.
    //  */
    // public static function Mask( $Mask,&$Base )
    // {
    //     if( $Mask['IsRoot'] === TRUE || $Base['IsRoot'] === TRUE )
    //         return;

    //     $Base['Segments'] = array_values(array_diff_assoc($Base['Segments'],$Mask['Segments']));
    //     if( empty($Base['Segments']) )
    //     {
    //         $Base['Segments'][0] = $Base['Separator'];
    //         $Base['IsDir'] = $Base['IsRoot'] = TRUE;
    //     }
    // }

    // /**
    //  * Determine if a string appears to be an absolute path.
    //  *
    //  * A string is considered an absolute path under the following conditions:
    //  *  - The first character is a forward slash or a backslash.
    //  *  - The second character is a colon (for Windows paths).
    //  *
    //  * @param string $path The string to check.
    //  * @throws Exception path is not a string.
    //  * @retval boolean TRUE if the string is an absolute path.
    //  */
    // public static function IsAbs( $path )
    // {
    //     if( is_string($path) === FALSE )
    //         throw new Exception("path is not a string.");

    //     return ((strpos($path,'/')===0) || (strpos($path,'\\')===0) || (strpos($path,':')===1));
    // }

    // /**
    //  * Return TRUE if $Child is a child path of $Parent.
    //  *
    //  * The follow semantics also apply:
    //  *  - If both $Child and $Parent IsRoot is TRUE, this returns FALSE.
    //  *  - If $Child IsRoot is TRUE this returns FALSE.
    //  *  - If $Parent IsRoot is TRUE this returns TRUE.
    //  *
    //  * @param path $Child The child path.
    //  * @param path $Parent The parent path.
    //  * @retval boolean TRUE if $Child is a child of $Parent.
    //  */
    // public static function IsChild( $Child,$Parent )
    // {
    //     if( ($Child['IsRoot'] === TRUE && $Parent['IsRoot'] === TRUE) || ($Child['IsRoot'] === TRUE) )
    //         return FALSE;
    //     else if( $Parent['IsRoot'] === TRUE )
    //         return TRUE;

    //     return (count(array_intersect_assoc($Child['Segments'],$Parent['Segments'])) === count($Parent['Segments']));
    // }

    // /**
    //  * Add a segment to the end of the path.
    //  *
    //  * @param string $Element The segment to append.
    //  * @param path $Subject The path to append to.
    //  * @retval int The position, counting from 0, at which the element was appended.
    //  */
    // public static function Append( $Element,&$Subject )
    // {
    //     $Element = trim($Element);
    //     if( substr($Element,-1,1) === $Subject['Separator'] )
    //         $Subject['IsDir'] = TRUE;
    //     else
    //         $Subject['IsDir'] = FALSE;

    //     $Element = trim($Element,$Subject['Separator']);
    //     if( empty($Element) )
    //         return NULL;

    //     $Subject['Segments'] = $Subject['IsRoot']===TRUE?array():$Subject['Segments'];

    //     return parent::Append($Element,$Subject['Segments']);
    // }

    // /**
    //  * Add a segment to the beginning of the path.
    //  *
    //  * @param string $Element The segment to prepend.
    //  * @param path $Subject The path to prepend to.
    //  * @retval int 0
    //  */
    // public static function Prepend( $Element,&$Subject )
    // {
    //     $Element = trim($Element);

    //     // TODO: is this needed?  should we assume that the common case
    //     // is to not change whether the path is absolute
    //     if( substr($Element,0,1) === $Subject['Separator'] )
    //         $Subject['IsAbs'] = TRUE;

    //     $Element = trim($Element,$Subject['Separator']);
    //     if( empty($Element) )
    //         return NULL;

    //     $Subject['Segments'] = $Subject['IsRoot']===TRUE?array():$Subject['Segments'];

    //     return parent::Prepend($Element,$Subject['Segments']);
    // }

    // /**
    //  * Insert a new segment after another segment.
    //  *
    //  * @param string $Element The segment to insert.
    //  * @param int $RefPoint Reference point of insertion, starting from 0.
    //  * @param path &$Subject The path to insert into.
    //  */
    // public static function InsertAfter( $Element,$RefPoint,&$Subject )
    // {
    //     return parent::InsertAfter($Element,$RefPoint,$Subject['Segments']);
    // }

    // /**
    //  * Insert a new segment before another segment.
    //  *
    //  * @param string $Element The segment to insert.
    //  * @param int $RefPoint Reference point of insertion, starting from 0.
    //  * @param path &$Subject The path to insert into.
    //  */
    // public static function InsertBefore( $Element,$RefPoint,&$Subject )
    // {
    //     return parent::InsertBefore($Element,$RefPoint,$Subject['Segments']);
    // }

    // /**
    //  * Get a segment from the path.
    //  *
    //  * See note for bottom - it may be appropriate to put ability to fetch
    //  * last, second-last, etc (perhaps modeled after substr().
    //  *
    //  * Right now it's a quick hack to mean a negative Needle to count in from the end.
    //  * No type of "length" is available (only one segment is returned).
    //  *
    //  * @param int $Needle Numeric index of segment, starting from 0.  A negative number will be counted from the end.
    //  * @param path $Haystack The path to read the segment from.
    //  * @param mixed $Check Optional value to check the segment against.
    //  */
    // public static function Get( $Needle,$Haystack,$Check = NULL )
    // {
    //     if( $Needle < 0 )
    //         return parent::Get(count($Haystack['Segments'])+$Needle,$Haystack['Segments'],$Check);
    //     else
    //         return parent::Get($Needle,$Haystack['Segments'],$Check);
    // }

    // /**
    //  * Prepend or append a segment in a path.
    //  *
    //  * $Needle is a string defining the change to make:
    //  *  @li @c <segment: prepend the segment.
    //  *  @li @c >segment: append the segment.
    //  *
    //  * @param string $Needle Direction and segment to add.
    //  * @param array $Haystack path Struct.
    //  * @retval int The position the segment was added.
    //  * @retval NULL Operation not recognized.
    //  *
    //  * @todo This may be expanded slightly.
    //  */
    // public static function Set( $Needle,&$Haystack )
    // {
    //     $Needle = trim($Needle);

    //     if( $Haystack['IsRoot'] === TRUE )
    //     {
    //         $Haystack['IsRoot'] = FALSE;
    //         $Haystack['Segments'][0] = ltrim(ltrim($Needle,'<'),'>');
    //         return count($Haystack['Segments']);
    //     }

    //     if( $Needle[0] === '<' )
    //         return static::Prepend(ltrim($Needle,'<'),$Haystack);
    //     else if( $Needle[0] === '>' )
    //         return static::Append(ltrim($Needle,'>'),$Haystack);
    //     else
    //         return NULL;
    // }

    // /**
    //  * Delete a segment by position.
    //  *
    //  * @param int $Needle The position of the segment, counting from 0.
    //  * @param array $Haystack path Struct.
    //  */
    // public static function Del( $Needle,&$Haystack )
    // {
    //     return parent::Del($Needle,$Haystack['Segments']);
    // }

    // /**
    //  * Read one or more path segments from the top.
    //  *
    //  * In a path such as /one/two/three, "one" is the top most segment.
    //  *
    //  * @param array $Haystack path Struct.
    //  * @param int $Limit Optional number of segments to read, starting from 1.
    //  * @retval string The single top-most path segment.
    //  * @retval array The specified number of top-most path segments as a new path Struct.
    //  */
    // public static function Top( $Haystack,$Limit = 0 )
    // {
    //     if( $Limit > 0 )
    //     {
    //         $H2 = $Haystack;
    //         $H2['Segments'] = array_slice($Haystack['Segments'],0,$Limit);
    //         return $H2;
    //     }
    //     else
    //         return $Haystack['Segments'][0];
    // }

    // /**
    //  * Read one or more path segments from the bottom.
    //  *
    //  * In a path such as /one/two/three, "three" is the bottom most segment.
    //  *
    //  * @param array $Haystack path Struct.
    //  * @param int $Limit Optional number of segments to read, starting from 1.
    //  * @retval string The single bottom-most path segment.
    //  * @retval array The specified number of bottom-most path segments as a new path Struct.
    //  */
    // public static function Bottom( $Haystack,$Limit = 0 )
    // {
    //     if( is_string($Haystack) )
    //         $Haystack = self::Init($Haystack);

    //     if( $Limit > 0 )
    //     {
    //         $H2 = $Haystack;
    //         $H2['Segments'] = array_slice($Haystack['Segments'],count($Haystack['Segments'])-$Limit);
    //         return $H2;
    //     }
    //     else
    //         return $Haystack['Segments'][count($Haystack['Segments'])-1];
    // }

    // /**
    //  * Iterate through a path segment by segment, left to right or right to left.
    //  *
    //  * This creates an array containing increasingly more or less specific versions of
    //  * the same path.
    //  *
    //  * By default this returns an array in increasing path size, i.e. most general to
    //  * most specific.
    //  *
    //  * @param array $path The path Struct.
    //  * @param boolean $Inc FALSE to iterate in decreasing path size, i.e. most specific to most general.
    //  * @retval array Ordered path segments.
    //  *
    //  * @note This doesn't honor IsDir or IsAbs of the path struct - there will always be leading
    //  *       and trailing separators on all segments.
    //  */
    // public static function Order( $path,$Inc = TRUE )
    // {
    //     if( path::IsA($path) === FALSE )
    //         throw new Exception('path is not a path Struct');

    //     if( $path['IsRoot'] === TRUE )
    //         return array($path['Separator']);

    //     $P = array();
    //     foreach( $path['Segments'] as $K => $V )
    //         $P[] = ($K>0?$P[$K-1]:(($path['Separator']))).$V.$path['Separator'];

    //     if( $Inc === TRUE )
    //         return $P;
    //     else
    //         return array_reverse($P);
    // }

