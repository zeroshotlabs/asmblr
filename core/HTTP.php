<?php
/**
 * @file HTTP.php URL, `asmblr application controller.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;



/**
 * Tools for manipulating a hostname.
 *
 * The structure of a Hostname is 0 through N subdomain parts,
 * counting from the TLD (root first, the same as \asm\Path).
 *
 * For example www.asmblr.org would be represented internally as:
 *
 *  @li @c 0: org
 *  @li @c 1: asmblr
 *  @li @c 2: www
 *
 * @note This does not support IDNA, nor checks for validity.
 * @note This does not include a port - @see URL.
 */
class Hostname extends DAO
{
    /**
     * Create a new Hostname object.
     * 
     * No validation is done amongst the arguments.
     * 
     * Use Hostname::str() instead.
     */
    public function __construct( public readonly array $Subdomains = [] )
    { }

    /**
     * Create a new Hostname from a string.
     *
     * @param string $HostnameStr The hostname string to parse.
     * @return \asm\Hostname A Hostname DAO.
     * 
     * @note The entire hostname is lowercased and it's NOT encoded.
     */
    public static function str( $HostnameStr )
    {
        if( !is_string($HostnameStr) || !empty($HostnameStr = trim($HostnameStr)) )
        {
            _stde("Hostname::str() - HostnameStr is not a string (".gettype($HostnameStr).")");
            return NULL;
        }

        return new self(array_reverse(explode('.',strtolower(trim($HostnameStr,'. ')))));
    }

    /**
     * Overload to return the hostname as a string.
     */
    public function __toString(): string
    {
        return implode('.',array_reverse($this->Subdomains));
    }
}


/**
 * Tools for working with a UNIX or URL path.
 *
 * By default, a Path uses the forward slash @c / as a separator.  While the
 * back slash can be used, no handling of special Windows paths or drives is done.
 *
 * A Segment is what's contained between two separators, or a separator
 * and the end of the string, which implies a directory.
 *
 * @note No automatic encoding/decoding/escaping is done.
 * @note This is platform agnostic - it doesn't know if it's running under Windows or Unix.
 * @note This uses read-only and is not reusable.
 * @note This is not a security mechanism - use realpath().
 */
class Path extends DAO
{
    /**
     * Create a new Path object.
     * 
     * No validation is done amongst the arguments.
     * 
     * Use Path::path() or Path::url() instead.
     */
    public function __construct(
        /**
         * The path's separator.
         */
        public readonly string $Separator = '/',

        /**
         * TRUE if the path has a leading separator.
         */
        public readonly bool $IsABS = null,

        /**
         * TRUE if the path has a trailing separator.
         */
        public readonly bool $IsDir = null,

        /**
         * TRUE if the path is only the separator (IsDir and IsAbs will also be TRUE).
         */
        public readonly bool $IsRoot = null,

        /**
         * TRUE if the path is a shell path, otherwise it's escaped as a URL path.
         */
        public readonly bool $IsShell = null,

        /**
         * Numeric array of the pieces between the separators.
         */
        public readonly array $Segments = []
    )
    { }

    /**
     * Overload to return the path as a string.
     * 
     * This will properly encode the path string, according to $IsShell.
     */
    public function __toString(): string
    {
        if( $this->IsRoot === TRUE )
        {
            return $this->Separator;
        }
        else
        {
            if( $this->IsShell === FALSE )
                $Segs = implode($this->Separator,array_map('rawurldecode',$this->Segments));
            else
                $Segs = implode($this->Separator,array_map('escapeshellcmd',$this->Segments));

            return ($this->IsABS?$this->Separator:'').$Segs.($this->IsDir?$this->Separator:'');
        }
    }

    /**
     * Create a filesystem Path from a string.
     *
     * A backslash separator is automatically detected if there is one, otherwise a forward
     * slash is the default.
     *
     * @param string $PathStr The path string to parse, an empty string, or NULL.
     * @param string $Separator Specify a single character as a separator to use.
     * @param bool $Shell TRUE if the path is a shell path, otherwise it'll be escaped as a URL path.
     * @return \asm\Path A Path DAO.
     *
     * @note An empty or NULL $PathStr, or one that is simply multiple separators,
     * 		 will be considered a root path.
     */
    public static function path( $PathStr,$Separator = NULL,$Shell = true ): \asm\Path
    {
        if( !is_string($PathStr) || !empty($PathStr = trim($PathStr)) )
        {
            _stde("Path::path() - PathStr is not a string (".gettype($PathStr).")");
            return NULL;
        }

        if( empty($Separator) )
            if( strpos($PathStr,'\\') !== FALSE )
                $Separator = '\\';
        else
            $Separator = '/';

        $Segments = [];

        // a root path
        if( empty($PathStr) || $PathStr === $Separator || trim($PathStr,$Separator) === '' )
        {
            $Segments[0] = $Separator;
            $IsAbs = $IsDir = $IsRoot = TRUE;
        }
        else
        {
            $IsRoot = FALSE;
            $IsAbs = $PathStr[0]===$Separator?TRUE:FALSE;
            $IsDir = substr($PathStr,-1,1)===$Separator?TRUE:FALSE;
            $Segments = preg_split("(\\{$Separator}+)",$PathStr,-1,PREG_SPLIT_NO_EMPTY);
        }

        return new self($Separator,$IsAbs,$IsDir,$IsRoot,$Shell,$Segments);
    }

    /**
     * Create a URL Path from a string.
     * 
     * @param string $PathStr The path string to parse, an empty string, or NULL.
     * @see \asm\Path::path()
     */     
    public static function url( $PathStr ): \asm\Path
    {
        return static::path($PathStr,'/',false);
    }

    /**
     * Make all Segments lowercase.
     * 
     * Chainable.
     */
    public function lower(): \asm\Path
    {
        $this->Segments = array_map('strtolower',$this->Segments);
        return $this;
    }
}


/**
 * Tools for manipulating encoded key/value pairs, such as GET query strings and POST data.
 *
 * @note This should not be used for multipart/form-data (PHP_QUERY_RFC1738).
 * @note This uses http_build_query() with PHP_QUERY_RFC3986 (rawurlencode()).
 */
class QueryString extends DAO
{
    public function __construct( public readonly $Pairs )
    { }

    public function __get( $Label ): mixed
    {
        return isset($this->Pairs[$Label])?$this->Pairs[$Label]:null;
    }

    public function __set( $Label,$Value ): void
    {
        $this->Pairs[$Label] = $Value;
    }

    public function count(): int
    {
        return count($this->Pairs);
    }

    /**
     * Create a new QueryString DAO from a string.
     *
     * @param string $URLEncStr The URL encoded string to parse.
     * @return \asm\QueryString A QueryString DAO.
     *
     * @note This uses parse_str().
     */
    public static function str( $QueryString ): \asm\QueryString
    {
        if( !is_string($QueryString) || !empty($QueryString = trim($QueryString)) )
        {
            _stde("QueryString::str() - QueryStringis not a string (".gettype($QueryString).")");
            return null;
        }

        $Q = [];
        parse_str($QueryString,$Q);
        return new self($Q);
    }

    /**
     * Create a string from the QueryString.
     *
     * @return string The URL encoded query string.
     *
     * @note This uses http_build_query() with the PHP_QUERY_RFC3986 encode type.
     * @note A '?' is automatically prefixed.
     */
    public function __toString(): string
    {
        if( !empty($this) )
            return '?'.http_build_query($this->Pairs,'','&',PHP_QUERY_RFC3986);
        else
            return '';
    }
}


/**
 * Tools for manipulating a URL.
 *
 * The components of a URL are:
 *
 *  @li @c IsHTTPS: @c TRUE if the scheme is https.
 *  @li @c Scheme: Typically @c http or @c https.
 *  @li @c Username
 *  @li @c Password
 *  @li @c Hostname
 *  @li @c Port
 *  @li @c Path: A @c Path Struct
 *  @li @c Query: A @c URLEncoded Struct
 *  @li @c Fragment or @c #
 *
 * @note This does not support full URL/URI pedantics and basically for HTTP.
 */
class URL extends DAO
{
    // protected static $Skel = array('IsHTTPS'=>FALSE,'Scheme'=>'','Username'=>'','Password'=>'',
    // 							   'Hostname'=>array(),'Port'=>'','Path'=>array(),'Query'=>array(),'Fragment'=>'');

    /**
     * TRUE if the scheme is https.
     */
    public readonly bool $IsHTTPS;

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
        public readonly string $Scheme,

        public readonly string $Username,

        public readonly string $Password,

        public readonly \asm\Hostname $Hostname,

        public readonly int $Port,

        public readonly \asm\Path $Path,

        public readonly \asm\QueryString $Query,

        public readonly string $Fragment
    )
    {
        if( $Scheme === 'https' )
            $this->IsHTTPS = true;
        else
            $this->IsHTTPS = false;
    }

    /**
     * Create a new URL DAO from a string.
     *
     * This uses parse_url() and follows it's semantics, whereby some URL strings will be indeterminate.
     * 
     * @param string $URLStr The URL string to parse.
     * @throws Exception Malformed URL '$URLStr' (parse_url()).
     * @return \asm\URL A URL DAO.
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

        $Scheme = strtolower($T['scheme'] ?? 'https');
        $Hostname = Hostname::str($T['host'] ?? '');
        $Port = $T['port'] ?? 0;

        $Username = $T['user'] ?? '';
        $Password = $T['pass'] ?? '';

        $Path = Path::url($T['path'] ?? '');

        $Query = QueryString::str($T['query'] ?? '');
        $Fragment = $T['fragment'] ?? '';

        return new self($Scheme,$Username,$Password,$Hostname,$Port,$Path,$Query,$Fragment);
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
        $host = (string) $this->Hostname;
        if( !empty($host) )
        {
            if( $this->Port && $this->Port !== 80 && $this->Port !== 443 )
                $host .= ":{$this->Port}";

            $auth = '';
            if( !empty($this->Username) )
            {
                $auth = rawurlencode($this->Username);
                if( !empty($this->Password) )
                    $auth .= ':'.rawurlencode($this->Password);
                $auth .= '@';
            }
            $host = "{$this->Scheme}://{$auth}{$host}";
        }

        $path = (string) $this->Path;
        $query = (string) $this->Query;
        $frag = !empty($this->Fragment) ? '#'.rawurlencode($this->Fragment) : '';

        return "{$host}{$path}{$query}{$frag}";

        // // if a hostname is present, ensure a / for relative paths - otherwise use what Path has
        // $Str .= (!empty($Str)&&empty($URL['Path']['IsAbs'])?'/':'').Path::ToString($URL['Path'],'url');
        // $Str .= empty($URL['Query'])?'':URLEncoded::ToString($URL['Query']);
        // $Str .= $URL['Fragment']===''?'':'#'.rawurlencode($URL['Fragment']);
    }
}



    // /**  FROM QUERYSTRING
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
    //  * @param string|NULL $Username The username, an empty string, or NULL.
    //  * @param string|NULL $Password The password, an empty string, or NULL.
    //  * @retval string URL authority string, which may be an empty string.
    //  *
    //  * @note A password without a username will return an empty string.
    //  * @note Explicitly setting a username in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function AuthorityToString( $Username,$Password )
    // {
    //     if( $Username !== NULL && $Username !== '' )
    //         return rawurlencode($Username).($Password!==''&&$Password!==NULL?':'.rawurlencode($Password).'@':'@');
    //     else
    //         return '';
    // }
    // /**
    //  * Change parts of a URL Struct.
    //  *
    //  * $Needle is a change string or array of strings defining the changes to make:
    //  *  - @c >segment: append a segment to the path.
    //  *  - @c <segment: prepend a segment to the path.
    //  *  - @c ?key=value: set a key/value in the query string.
    //  *  - @c ?key=: delete a key/value in the query string.
    //  *  - @c ?: delete entire query string.
    //  *  - @c \#fragment: set the URL fragment.
    //  *  - @c #: delete the URL fragment.
    //  *
    //  * $Needle may also combine segment, ?key=value and \#fragment change strings:
    //  *  - \c >new-segment?login=1&register=&redirect=1
    //  *
    //  * This would append a new path segment, set login and redirect query variables
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
    //             URLEncoded::Set($V,$Haystack['Query']);
    //             continue;
    //         }
    //         else if( !is_int($K) )
    //         {
    //             URLEncoded::Set(array(array($K=>$V)),$Haystack['Query']);
    //             continue;
    //         }

    //         $V2 = explode('#',$V);
    //         if( isset($V2[1]) )
    //             static::SetFragment($V2[1],$Haystack);

    //         $V2 = explode('?',$V2[0]);
    //         if( isset($V2[1]) )
    //             URLEncoded::Set($V2[1],$Haystack['Query']);

    //         if( !empty($V2[0]) )
    //         {
    //             if( ($V2[0][0] === '>' || $V2[0][0] === '<') )
    //             {
    //                 Path::Set($V2[0],$Haystack['Path']);
    //                 $Haystack['Path']['IsAbs'] = TRUE;
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
    // public static function Hostname( $URL )
    // {
    //     return Hostname::ToString($URL['Hostname']);
    // }

    // /**
    //  * Read the full path as a string.
    //  *
    //  * @param array $URL URL Struct.
    //  * @retval string The path string.
    //  */
    // public static function Path( $URL )
    // {
    //     return Path::ToString($URL['Path'],'url');
    // }

    // /**
    //  * Create a new URL from individual parts.
    //  *
    //  * @param string $Scheme Typically http or https (:// is trimmed).
    //  * @param string $Hostname Hostname.
    //  * @param string|array $Path Path string or Path Struct.
    //  * @param string|array $Query Query string or URLEncoded Struct.
    //  * @param string|NULL $Port Optional port.
    //  * @param string|NULL $Username Optional username.
    //  * @param string|NULL $Password Optional password.
    //  * @param string|NULL $Fragment Optional fragment.
    //  * @retval array The URL.
    //  *
    //  * @note Explicitly setting a username/password in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function InitParts( $Scheme,$Hostname,$Path,$Query,$Port = NULL,$Username = NULL,$Password = NULL,$Fragment = NULL )
    // {
    //     $URL = static::$Skel;

    //     static::SetScheme($Scheme,$URL);
    //     static::SetHostname($Hostname,$URL);
    //     static::SetPath($Path,$URL);
    //     static::SetQuery($Query,$URL);

    //     if( $Port !== NULL )
    //         static::SetPort($Port,$URL);
    //     if( $Username !== NULL )
    //         static::SetUsername($Username,$URL);
    //     if( $Password !== NULL )
    //         static::SetPassword($Password,$URL);
    //     if( $Fragment !== NULL )
    //         static::SetFragment($Fragment,$URL);

    //     return $URL;
    // }

    // /**
    //  * Change the scheme of a URL.
    //  *
    //  * @param string $Scheme A string.
    //  * @param array $URL A URL Struct.
    //  *
    //  * @note This will affect the URL's IsHTTPS value.
    //  */
    // public static function SetScheme( $Scheme,&$URL )
    // {
    //     $URL['Scheme'] = strtolower(trim(trim($Scheme,'://')));
    //     $URL['IsHTTPS'] = $URL['Scheme']==='https'?TRUE:FALSE;
    // }

    // /**
    //  * Change the Hostname of a URL.
    //  *
    //  * @param string|array $Hostname A string or Hostname Struct to set.
    //  * @param array $URL A URL Struct.
    //  * @throws Exception Hostname not a string or Hostname Struct.
    //  */
    // public static function SetHostname( $Hostname,&$URL )
    // {
    //     if( is_string($Hostname) === TRUE )
    //         $URL['Hostname'] = Hostname::Init($Hostname);
    //     else if( is_array($Hostname) === TRUE )
    //         $URL['Hostname'] = $Hostname;
    //     else
    //         throw new Exception("Hostname not a string or Hostname Struct.");
    // }

    // /**
    //  * Change the Path of a URL.
    //  *
    //  * @param string|Path $Path A string or Path Struct to set.
    //  * @param array $URL A URL Struct.
    //  * @throws Exception Path not a string or Path Struct.
    //  */
    // public static function SetPath( $Path,&$URL )
    // {
    //     if( is_string($Path) === TRUE )
    //         $URL['Path'] = Path::Init($Path);
    //     else if( isset($Path['Segments']) === TRUE )
    //         $URL['Path'] = $Path;
    //     else if( $Path !== NULL )
    //         throw new Exception('Path not a string or Path Struct.');
    // }

    // /**
    //  * Change the Query of a URL.
    //  *
    //  * @param string|array $Query A string or URLEncoded Struct to set.
    //  * @param array $URL A URL Struct.
    //  * @throws Exception Query not a string or URLEncoded Struct.
    //  */
    // public static function SetQuery( $Query,&$URL )
    // {
    //     if( is_string($Query) === TRUE )
    //         $URL['Query'] = URLEncoded::Init($Query);
    //     else if( is_array($Query) === TRUE )
    //         $URL['Query'] = $Query;
    //     else if( $Query !== NULL )
    //         throw new Exception("Query not a string or URLEncoded Struct.");
    // }

    // /**
    //  * Change the Port of a URL.
    //  *
    //  * @param string|int $Port A string or integer.
    //  * @param URL $URL A URL Struct.
    //  *
    //  * @note The port is always converted to a string.
    //  * @note If the port is 80 or 443, it is set as the empty string.
    //  */
    // public static function SetPort( $Port,&$URL )
    // {
    //     if( in_array((string)$Port,array('','80','443')) === TRUE )
    //         $URL['Port'] = '';
    //     else
    //         $URL['Port'] = (string) $Port;
    // }

    // /**
    //  * Change the Username of a URL.
    //  *
    //  * @param string $Username A string.
    //  * @param array $URL A URL Struct.
    //  *
    //  * @note Explicitly setting a username in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function SetUsername( $Username,&$URL )
    // {
    //     $URL['Username'] = $Username===NULL?$URL['Username']:$Username;
    // }

    // /**
    //  * Change the Password of a URL.
    //  *
    //  * @param string $Password A string.
    //  * @param array $URL A URL Struct.
    //  *
    //  * @note A Username check is not done.
    //  * @note Explicitly setting a password in a URL may be incompatible with some browsers, like IE.
    //  */
    // public static function SetPassword( $Password,&$URL )
    // {
    //     $URL['Password'] = $Password===NULL?$URL['Password']:$Password;
    // }

    // /**
    //  * Change or delete the Fragment of a URL.
    //  *
    //  * @param string $Fragment Fragment or a single # to delete an existing one.
    //  * @param array $URL A URL Struct.
    //  */
    // public static function SetFragment( $Fragment,&$URL )
    // {
    //     if( $Fragment === '#' )
    //         $URL['Fragment'] = '';
    //     else if( $Fragment !== NULL )
    //         $URL['Fragment'] = ltrim($Fragment,'#');
    // }



    //   WAS HOSTNAME
    // /**
    //  * Create a string from the Hostname array.
    //  *
    //  * @param array $Hostname A Hostname Struct.
    //  * @retval string The hostname string.
    //  */
    // public static function ToString( $Hostname )
    // {
    //     return implode('.',array_reverse($Hostname));
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
    //  * Prepend or append a subdomain in a Hostname.
    //  *
    //  * $Needle is a string defining the change to make:
    //  *  - \c <subdomain: prepend the subdomain.
    //  *  - \c >subdomain: append the subdomain.
    //  *
    //  * @param string $Needle Direction and subdomain to add.
    //  * @param array $Haystack Hostname Struct.
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
    //  * @param array $Hostname The Hostname Struct to search.
    //  * @retval int The position of the sub-domain.
    //  * @retval FALSE The sub-domain was not found.
    //  *
    //  * @note The search is case-sensitive.
    //  */
    // public static function Search( $Needle,$Hostname )
    // {
    //     return array_search($Needle,$Hostname);
    // }

    // /**
    //  * Read one or more sub-domains from the top.
    //  *
    //  * In a hostname such as www.asmblr.org, "org" is the top most sub-domain.
    //  *
    //  * @param array $Haystack Hostname Struct.
    //  * @param int $Limit Optional number of sub-domains to read, starting from 1.
    //  * @retval string The single top-most sub-domain.
    //  * @retval array The specified number of top-most sub-domains as a new Hostname Struct.
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
    //  * @param array $Haystack Hostname Struct.
    //  * @param int $Limit Optional number of sub-domains to read, starting from 1.
    //  * @retval string The single bottom-most sub-domain.
    //  * @retval array The specified number of bottom-most sub-domains as a new Hostname Struct.
    //  */
    // public static function Bottom( $Haystack,$Limit = 0 )
    // {
    //     if( $Limit > 0 )
    //         return array_slice($Haystack,count($Haystack)-$Limit);
    //     else
    //         return $Haystack[count($Haystack)-1];
    // }


    // /**
    //  * Iterate through a Hostname sub-domain by sub-domain, left to right or right to left.
    //  *
    //  * This creates an array containing increasingly more or less specific versions of
    //  * the same hostname.
    //  *
    //  * By default this returns an array in increasing hostname size, i.e. most general to
    //  * most specific.
    //  *
    //  * @param array $Hostname The Hostname Struct.
    //  * @param boolean $Inc Iterate in increasing hostname size, i.e., most general to most specific.
    //  * @retval array Ordered hostname sub-domains.
    //  *
    //  * @note Each hostname returned will have a leading period and no trailing period.
    //  */
    // public static function Order( $Hostname,$Inc = TRUE )
    // {
    //     $P = array();
    //     foreach( $Hostname as $K => $V )
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
    //  * @param Path $Src The Path to merge in.
    //  * @param Path $Dest A reference to the base Path to merge into.
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
    //  * @param Path $Mask The Path that masks segments.
    //  * @param Path $Base The Path that will have segments removed.
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
    //  * @param string $Path The string to check.
    //  * @throws Exception Path is not a string.
    //  * @retval boolean TRUE if the string is an absolute path.
    //  */
    // public static function IsAbs( $Path )
    // {
    //     if( is_string($Path) === FALSE )
    //         throw new Exception("Path is not a string.");

    //     return ((strpos($Path,'/')===0) || (strpos($Path,'\\')===0) || (strpos($Path,':')===1));
    // }

    // /**
    //  * Return TRUE if $Child is a child path of $Parent.
    //  *
    //  * The follow semantics also apply:
    //  *  - If both $Child and $Parent IsRoot is TRUE, this returns FALSE.
    //  *  - If $Child IsRoot is TRUE this returns FALSE.
    //  *  - If $Parent IsRoot is TRUE this returns TRUE.
    //  *
    //  * @param Path $Child The child path.
    //  * @param Path $Parent The parent path.
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
    //  * @param Path $Subject The Path to append to.
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
    //  * @param Path $Subject The Path to prepend to.
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
    //  * @param Path &$Subject The Path to insert into.
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
    //  * @param Path &$Subject The Path to insert into.
    //  */
    // public static function InsertBefore( $Element,$RefPoint,&$Subject )
    // {
    //     return parent::InsertBefore($Element,$RefPoint,$Subject['Segments']);
    // }

    // /**
    //  * Get a segment from the Path.
    //  *
    //  * See note for bottom - it may be appropriate to put ability to fetch
    //  * last, second-last, etc (perhaps modeled after substr().
    //  *
    //  * Right now it's a quick hack to mean a negative Needle to count in from the end.
    //  * No type of "length" is available (only one segment is returned).
    //  *
    //  * @param int $Needle Numeric index of segment, starting from 0.  A negative number will be counted from the end.
    //  * @param Path $Haystack The Path to read the segment from.
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
    //  * Prepend or append a segment in a Path.
    //  *
    //  * $Needle is a string defining the change to make:
    //  *  @li @c <segment: prepend the segment.
    //  *  @li @c >segment: append the segment.
    //  *
    //  * @param string $Needle Direction and segment to add.
    //  * @param array $Haystack Path Struct.
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
    //  * @param array $Haystack Path Struct.
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
    //  * @param array $Haystack Path Struct.
    //  * @param int $Limit Optional number of segments to read, starting from 1.
    //  * @retval string The single top-most path segment.
    //  * @retval array The specified number of top-most path segments as a new Path Struct.
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
    //  * @param array $Haystack Path Struct.
    //  * @param int $Limit Optional number of segments to read, starting from 1.
    //  * @retval string The single bottom-most path segment.
    //  * @retval array The specified number of bottom-most path segments as a new Path Struct.
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
    //  * Iterate through a Path segment by segment, left to right or right to left.
    //  *
    //  * This creates an array containing increasingly more or less specific versions of
    //  * the same path.
    //  *
    //  * By default this returns an array in increasing path size, i.e. most general to
    //  * most specific.
    //  *
    //  * @param array $Path The Path Struct.
    //  * @param boolean $Inc FALSE to iterate in decreasing path size, i.e. most specific to most general.
    //  * @retval array Ordered path segments.
    //  *
    //  * @note This doesn't honor IsDir or IsAbs of the Path struct - there will always be leading
    //  *       and trailing separators on all segments.
    //  */
    // public static function Order( $Path,$Inc = TRUE )
    // {
    //     if( Path::IsA($Path) === FALSE )
    //         throw new Exception('Path is not a Path Struct');

    //     if( $Path['IsRoot'] === TRUE )
    //         return array($Path['Separator']);

    //     $P = array();
    //     foreach( $Path['Segments'] as $K => $V )
    //         $P[] = ($K>0?$P[$K-1]:(($Path['Separator']))).$V.$Path['Separator'];

    //     if( $Inc === TRUE )
    //         return $P;
    //     else
    //         return array_reverse($P);
    // }

