<?php declare(strict_types=1);
/**
 * @file types.php Base types, interfaces and classes.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License v3.0 or later.
 * @copyright See COPYRIGHT.txt.
 */
namespace asm\types;
use function asm\sys\_stde;
use asm\_e\e500;


/**
 * @todo not implemented
 */
interface extension
{

}


/**
 * General purpose data access objects.
 * 
 * @implements \ArrayAccess allows access syntax such as [] and $dataset['key'].
 * @implements \Countable which indicates the number of key/value pairs.
 * @implements \Iterator @todo  (daotab/daocol)
 */
class dao extends \ArrayObject
{
    public int $flags = \ArrayObject::ARRAY_AS_PROPS;

    /**
     * Instantiates a dao wrapped around an array.
     * 
     * @note Doesn't reference the original array.
     */
    public function __construct( $kv )
    {
        parent::__construct($kv,$this->flags);
    }

    public function __clone()
    {
        $this->exchangeArray($this->getArrayCopy());
    }

    // public function __isset( $key ): bool
    // {
    //     return isset($this->$key);
    // }

    // public function __get( string $key ): mixed
    // {
    //     return isset($this->$key)?$this->$key:null;
    // }

    // public function __set( string $key,mixed $value ): void
    // {
    //     $this->$key = $value;
    // }

}


/**
 * Define the encoding types available for query strings.
 * 
 * @see \asm\types\encoded_str
 */
enum str_encodings: int
{
    /**
     * Encoded as application/x-www-form-urlencoded (spaces become +).
     */
    case FORM = \PHP_QUERY_RFC1738;

    /**
     * URL encoded (spaces become %20)
     */
    case URL = \PHP_QUERY_RFC3986;
}


/**
 * Tools for manipulating RFC 1738/3986 query strings, such as for GET and most POST data.
 * 
 * By default the encoding is RFC 3986 (URLs) but can be changed upon instantiation.
 *
 * @note This should not be used for multipart/form-data (form uploads)).
 * @note This uses http_build_query() with PHP_QUERY_RFC3986 by default.
 * 
 * @todo https://www.php.net/manual/en/ref.url.php   ?
 * @todo should be cleaned up, more aligned with dao; implement countable
 */
class query_str extends dao implements \Stringable
{
    use encoded_str;
    
    // encodings::PHP_QUERY_RFC1738 (+/POST) or encodings::PHP_QUERY_RFC3986 (%20/query string)
    public str_encodings $_encoding = str_encodings::URL;


    /**
     * Create a new encoded from a string.
     *
     * @param string $str The encoded string to parse.
     * @return \asm\types\query_str A encoded query string.
     *
     * @note This uses parse_str().
     */
    public static function str( string $str ): self
    {
        $Q = [];
        parse_str($str,$Q);
        return new self($Q);
    }
    
    public function __toString(): string
    {
        return $this->__toString();
    }
}



/**
 * Tools for working with a UNIX or URL path.
 *
 * By default, a path uses the forward slash @c / as a separator.  While the
 * back slash can be used, no handling of special Windows paths or drives is done.
 *
 * A Segment is what's contained between two separators, or a separator
 * and the end of the string, which implies a directory.
 *
 * @note No automatic encoding/decoding/escaping is done.
 * @note This is platform agnostic - it doesn't know if it's running under Windows or Unix.
 * @note This uses read-only and is not reusable.
 * @note This is not a security mechanism - use realpath().
 * @todo could support setting specific segments using array notation (implement countable_array but needs to support $kv)
 * @todo no visibility pendantics; don't change things you don't know what they do
 */
class path extends dao implements \Stringable
{
    /**
     * Use path::path() or path::url() instead.
     */
    private function __construct(
        /**
         * The path's separator.
         */
        public string $separator = '/',

        /**
         * true if the path has a leading separator.
         */
        public bool $is_abs,

        /**
         * true if the path has a trailing separator.
         */
        public bool $is_dir,

        /**
         * true if the path is only the separator (is_dir and IsAbs will also be true).
         */
        public bool $is_root,

        /**
         * true if the path is a shell path, otherwise it's escaped as a URL path.
         */
        public bool $is_shell,

        /**
         * Numeric array of the pieces between the separators.
         */
        public array $segments = []
    )
    {
        parent::__construct($this->segments);
    }

    /**
     * Create a filesystem path from a string.
     *
     * A backslash separator is automatically detected if there is one, otherwise a forward
     * slash is the default.
     *
     * @param string $str The path string to parse, an empty string, or NULL.
     * @param string $separator Specify a single character as a separator to use.
     * @param bool $shell true if the path is a shell path, otherwise it'll be escaped as a URL path.
     * @return \asm\types\path A path.
     *
     * @note An empty or NULL $str, or one that is only multiple separators,
     * 		 will be considered a root path.
     * @note This doesn't pass through query string.
     */
    public static function path( string $str,string $separator = NULL,bool $shell = true ): path
    {
        $segments = [];
        $str = trim($str);

        if( empty($separator) )
        {
            if( strpos($str,'\\') !== false )
                $separator = '\\';
            else
                $separator = '/';
        }

        // a root path
        if( empty($str) || $str === $separator )
        {
            $segments[0] = $separator;
            $IsAbs = $is_dir = $is_root = true;
        }
        else
        {
            $is_root = false;
            $IsAbs = $str[0]===$separator?true:false;
            $is_dir = substr($str,-1,1)===$separator?true:false;
            // $segments = preg_split("(\\{$separator}+)",$str,-1,PREG_SPLIT_NO_EMPTY);
            // the fastest way according to chatgpt - after some guidance
            // the double reverse is to reindex (not from chatgpt :)
            $segments = array_reverse(array_reverse(array_filter(explode($separator,$str))));
        }

        return new self($separator,$IsAbs,$is_dir,$is_root,$shell,$segments);
    }


    /**
     * Create a URL path from a string.
     * 
     * @param string $str The path string to parse, an empty string, or NULL.
     * @see \asm\types\path::path()
     */     
    public static function url( $str ): path
    {
        return static::path($str,'/',false);
    }


    public function as_abs(): string
    {
        return $this->the_real_tostring(NULL,NULL,true);
    }

    public function as_dir(): string
    {
        return $this->the_real_tostring(NULL,true,NULL);
    }

    public function as_abs_dir(): string
    {
        return $this->the_real_tostring(NULL,true,true);
    }

    /**
     * Prepend segments of another path.
     * 
     * @param path $p The path to prepend.
     * @param bool $dedupe true to remove duplicates.
     * 
     * @note If $dedupe is true, pay attention to the order of the segments.
     */
    public function prepend( path $p,$dedupe = false ): void
    {
        if( $dedupe )
            $this->segments = array_unique(array_merge($p->segments,$this->segments));
        else
            $this->segments = array_merge($p->segments,$this->segments);
    }

    /**
     * Append segments of another path.
     * 
     * @param path $p The path to append.
     * @param bool $dedupe true to remove duplicates.
     * 
     * @note If $dedupe is true, pay attention to the order of the segments.
     * @todo mixed instead of path for $p
     */
    public function append( mixed $p,$dedupe = false ): void
    {
        $this->segments += $p->segments;

        if( $dedupe )
            $this->segments = array_unique($this->segments);
    }

    /**
     * Masks segments of another path.
     * 
     * @param path $p The path to use as the mask.
     * 
     * @note Double array_reverse() is to serialize keys.
     */
    public function mask( path $mask ): void
    {
        $this->segments = array_reverse(array_reverse(array_diff($this->segments,$mask->segments)));
    }

    /**
     * Iterate through a path segment by segment, right to left (children first, 
     * decreasing) or left to right (root first, increasing, default).
     *
     * @param bool $inc false to iterate in decreasing path size, ie., specific to general.
     * @return array Ordered path segments.
     *
     * @note All paths are absolute. A trailing '/' will be included, based
     * on is_dir.
     */
    public function ordered( $inc = true ): array
    {
        if( $this->is_root )
            return [$this->separator];

        $p = [];
        foreach( $this->segments as $k => $v )
            $p[] = ($k>0?$p[$k-1]:(($this->separator))).$v.$this->separator;            

        if( !$this->is_dir )
            $p[count($p)-1] = rtrim(end($p),$this->separator);

        if( $inc )
            return $p;
        else
            return array_reverse($p);
    }


    /**
     * Overload to return the path as a string.
     * 
     * This will properly prefix/suffix and encode the path string,
     * according to is_shell, is_abs and is_dir.
     * 
     * @todo PHP feature request: using python's slicing, printf formatting, etc?  Allow params?
     * @see the_real_tostring() for actual string creation and overrides.
     */
    public function __toString(): string
    {
        return $this->the_real_tostring();
    }

    protected function the_real_tostring( bool $is_shell = NULL,bool $is_dir = NULL,bool $is_abs = NULL ): string
    {
        if( $this->is_root === true )
        {
            return $this->separator;
        }
        else
        {
            $shell = $is_shell===NULL?$this->is_shell:$is_shell;
            $abs = $is_abs===NULL?$this->is_abs:$is_abs;
            $dir = $is_dir===NULL?$this->is_dir:$is_dir;

            if( $shell === false )
                $Segs = implode($this->separator,array_map('rawurldecode',$this->segments));
            else
                $Segs = implode($this->separator,array_map('escapeshellcmd',$this->segments));

            return ($abs?$this->separator:'').$Segs.($dir?$this->separator:'');
        }
    }

    
    /**
     * Make all segments lowercase.
     * 
     * Chainable.
     * 
     * @return \asm\types\path
     * @todo option to return new object that's lowercased
     */
    public function lower(): path
    {
        $this->segments = array_map('strtolower',$this->segments);
        return $this;
    }
}


/**
 * Tools for manipulating a hostname.
 *
 * The structure of a hostname is 0 through N subdomain parts,
 * counting from the TLD (root first, the same as \asm\path).
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
class hostname extends dao implements \Stringable
{
    /**
     * Use hostname::str() instead.
     */
    private function __construct( public readonly array $subdomains = [] )
    { }

    /**
     * Create a new hostname from a string.  No validation or encode/decode is done.
     *
     * @param string $hostnamestr The hostname string to parse.
     * @return \asm\types\hostname A hostname.
     * 
     * @note The entire hostname is lowercased and it's NOT encoded.
     */
    public static function str( $str ): hostname
    {
        if( !is_string($str) && !empty($str = trim($str)) )
            return new self;
        else
            return new self(array_reverse(explode('.',strtolower(trim($str,'. ')))));
    }

    /**
     * Return the hostname as a string.
     */
    public function __tostring(): string
    {
        return implode('.',array_reverse($this->subdomains));
    }
}


/**
 * A URL, including scheme, host and path.
 *
 * The components of a URL are:
 *
 *  @li @c IsHTTPS: @c true if the scheme is https.
 *  @li @c scheme: Typically @c http or @c https.
 *  @li @c username
 *  @li @c password
 *  @li @c hostname
 *  @li @c port
 *  @li @c path: A @c path Struct
 *  @li @c encoded: A @c URLEncoded Struct
 *  @li @c fragment or @c #
 *
 * @note This does not support full URL/URI, username, port, etc. pedantics and basically straightforward.
 * @note If port is specified as 80 or 443, it'll be set as an empty string and not included when output as a string.
 */
class url implements \Stringable
{
    /**
     * Generic for URL parts.
     */
    public static array $_generic = ['scheme'=>'','username'=>'',
                                     'password'=>'','hostname'=>'',    // \asm\types\hostname
                                     'port'=>'','path'=>'',            // \asm\types\path
                                     'encoded'=>'','fragment'=>''];    // \asm\types\query_str

    /**
     * true if the scheme is https.
     */
    public bool $https;

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
        public string $scheme,

        public string $username,

        public string $password,

        public \asm\types\hostname $hostname,

        public mixed $port,

        public \asm\types\path $path,

        public \asm\types\query_str $query_str,

        public string $fragment
    )
    {
        $this->https = ($scheme === 'https');
        $this->port = (int) (($port == 80 || $port == 443 ? 0 : $port));
    }

    public function __clone()
    {
        $this->hostname = clone $this->hostname;
        $this->path = clone $this->path;
        $this->query_str = clone $this->query_str;
    }

    public static function str( string $urlstr ): url
    {
        return new url(...self::from_string(($urlstr)));
    }

    /**
     * Parse a string into it's constituent URL parts.
     *
     * This uses parse_url() and follows it's semantics, whereby some URL strings will be indeterminate.
     * 
     * @param string $url_str The URL string to parse.
     * @throws Exception Malformed URL '$url_str' (parse_url()).
     * @return array Parts of the URL as defined by $components. 
     *
     * @note This uses parse_url() and defaults to https if not specified. It attempts
     * to detect and auto-correct malformed URLs but it's not perfect; filename.html is
     * parsed as domain.com for example.
     */
    public static function from_string( $url_str ): array
    {
        if( !is_string($url_str) || empty($url_str = trim($url_str)) )
        {
            _stde("URL::str() - url_str is not a string or empty (".gettype($url_str).")");
            return null;
        }

        // perform some preprocess for domain vs path vs URL detection magic if there isn't a scheme
        if( strpos($url_str,'://') === false )
        {
            $p_p = strpos($url_str,'.');
            $p_s = strpos($url_str,'/');

            // a period before a slash, or no slash - prepend https://
            // domain.com  domain.com/  domain.com/something.html
            if( $p_p < $p_s || $p_s === false )
            {
                $url_str = "https://{$url_str}";
            }
            // a slash before a period or slash at first character - treat as an absolute path only
            else if( $p_s < $p_p || $p_s === 0 )
            {
                $url_str = '/'.$url_str;
            }
        }

        if( ($T = @parse_url(trim($url_str))) === false )
            throw new e500("Malformed URL '$url_str' (parse_url()).");

        $scheme = strtolower($T['scheme'] ?? 'https');
        $hostname = hostname::str($T['host'] ?? '');
        $port = $T['port'] ?? '';

        $username = $T['user'] ?? '';
        $password = $T['pass'] ?? '';

        $path = path::url($T['path'] ?? '');

        $encoded = query_str::str($T['encoded'] ?? '');
        $fragment = $T['fragment'] ?? '';

        return [$scheme,$username,$password,$hostname,$port,$path,$encoded,$fragment];
    }

    /**
     * Create a URL string depending on the URL parts present.
     *
     * @return string URL string.
     *
     * @note Logic exists to handle an empty hostname in which case scheme/port/username/password isn't included
     *       and thus a path-only "URL" URI is returned.
     */
    public function __toString(): string
    {
        $host = (string) $this->hostname;
        if( !empty($host) )
        {
            if( $this->port && ($this->port !== 80 && $this->port !== 443) )
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
        $query_str = (string) $this->query_str;
        $frag = !empty($this->fragment) ? '#'.rawurlencode($this->fragment) : '';

        // @todo confirm that this correctly returns partial URLs, like a path only, etc. per below
        return "{$host}{$path}{$query_str}{$frag}";

        // // if a hostname is present, ensure a / for relative paths - otherwise use what path has
        // $Str .= (!empty($Str)&&empty($URL['path']['IsAbs'])?'/':'').path::ToString($URL['path'],'url');
        // $Str .= empty($URL['encoded'])?'':URLEncoded::ToString($URL['encoded']);
        // $Str .= $URL['fragment']===''?'':'#'.rawurlencode($URL['fragment']);
    }
}



/**
 * 
 * 
 */

trait linker
{
//    use tokened;
    
    /**
     * Array of key/values representing a name and destination path.
     * 
     * Full URLs are generated based on the other parts of the URL.
     * 
     * These are typically app endpoints or FES prefixes.
     */
    public array $link_dests = [];

    public function __construct( string|url $base_url )  // change string
    {
        if( is_string($base_url) )
        {
            $base_url = url::str($base_url);
            if( $base_url === null )
                throw new e500("Malformed URL '$base_url' (parse_url()).");
        }
    }

}

/**
 * URLs support "change tokens" which can be used to change the URL on an one-off
 * temporary basis, for example when generating links.
 * 
 * Each token makes a change to a part of the URL, which can be accumalated as an array,
 * in which case they are applied in the order they appear.
 * 
 *  - query string: change or remove a key/value pair.
 *      [key,value] will change the value of the key.
 *      [key,''|null] will remove a key.
 * 
 *  - path: change or remove a path segment, counted from zero.
 *      [index,value] will change the value of the segment
 *      [index,''|null] will remove a segment.
 *                  
 * - fragment: change or remove the fragment.   
 * - fragment: change or remove the fragment.
 */

/**
 * Tokens are class-specific actions and parameters, typically as key/value pairs
 * where the value can be non-scalar.
 *
 * They are often used to represent configuration or containered actions and parameters,
 * and commonly represented as query or JSON strings.
 */
class token
{
    public function __construct( public string $name,
                                 public string $type,
                                 public mixed $value )
    {

    }
}



/**
 * Create well-formed URLs based on endpoints and FES resources.
 *
 * URLs are generated based on $base_url, which defaults to the current fully normalized
 * request URL, which can be modified, either for the lifetime of the object, or per-URL
 * generation using "change-strings."
 * 
 * URLs can be generated by calling a method name corresponding to an endpoint or FES resource,
 * or by calling the object itself, both of which support change-strings.
 * 
 * Additionally, a "change-string" can be passed, which will change the generated URL on
 * the fly based on the below mechanics.
 * 
 * 
 *  A LinkSet creates URLs calculated from a base URL.  Created URLs may also contain
 * one-time changes, merged on the fly at the time of creation.
 *
 * A LinkSet is instantiated with a base URL and optional array of changes:
 *     $ls = new LinkSet('www.stackware.com',array('district'=>'nyc'));
 *     $ls = new LinkSet('www.stackware.com','district=nyc');
 *
 * Both lines do the same thing - all created URLs will be, by default, based on:
 *     http://www.stackware.com/?district=nyc
 *
 * The $ls object is then called as a function with additional changes for creating the URLs:
 *     $ls('<login');
 *     $ls('>login');
 *
 * These prepend and append, respectively, a path segment and thus would produce the
 * same URL in this example (our base URL has a root path):
 *     http://www.stackware.com/login?district=nyc
 *
 * An array of change strings may be also be used.
 *
 * @see URL::Set() for details on the change string syntax.
 * @note All path segments and query string keys/values are properly encoded.
 *       Hostname/port/scheme are not encoded.
 */

 
//     /**
//      * Form URL and perform a permanent redirect to it.
//      *
//      * @param string $File A filename with optional path or an empty string to use only the base URL.
//      * @param array $Set Optional on-the-fly URL changes to apply.
//      */
//     public function Go( $File = NULL,$Set = array() )
//     {
//         HTTP::Location($this->__invoke($File,$Set));
//     }
// }


//     /**
//      * Return the base URL as a string.
//      *
//      * @retval string The current BaseURL.
//      */
//     public function __toString()
//     {
//         return URL::ToString($this->BaseURL);
//     }

//     /**
//      * Build a URL for the provided Path or filename.
//      *
//      * The URLs are calculated from BaseURL and may incorporate one-time changes.
//      *
//      * @param string $File A filename with optional path or an empty string to use only the base URL.
//      * @param array $Set Array of change strings for one-time changes to BaseURL.
//      * @retval string A well-formed URL.
//      */
//     public function __invoke( $File = '',$Set = array() )
//     {
//         $Base = $this->BaseURL;

//         if( !empty($File) )
//             Path::Merge(Path::Init($File),$Base['Path']);

//         if( !empty($Set) )
//             URL::Set($Set,$Base);

//         return URL::ToString($Base);
//     }


// -    public static function Set( $Needle,&$Haystack )
// -    {
// -        $Needle = trim($Needle);
// -
// -        if( $Needle[0] === '<' )
// -            return static::Prepend(ltrim($Needle,'<'),$Haystack);
// -        else if( $Needle[0] === '>' )
// -            return static::Append(ltrim($Needle,'>'),$Haystack);
// -        else
// -            return NULL;
// -    }
// -    public static function Set( $Needle,&$Haystack )
// -    {
// -        $Needle = trim($Needle);
// -
// -        if( $Haystack['IsRoot'] === TRUE )
// -        {
// -            $Haystack['IsRoot'] = FALSE;
// -            $Haystack['Segments'][0] = ltrim(ltrim($Needle,'<'),'>');
// -            return count($Haystack['Segments']);
// -        }
// -
// -        if( $Needle[0] === '<' )
// -            return static::Prepend(ltrim($Needle,'<'),$Haystack);
// -        else if( $Needle[0] === '>' )
// -            return static::Append(ltrim($Needle,'>'),$Haystack);
// -        else
// -            return NULL;
// -    }

// public static function Set( $Needle,&$Haystack )
// -    {
// -        foreach( (array) $Needle as $K => $V )
// -        {
// -            if( is_array($V) )
// -            {
// -                URLEncoded::Set($V,$Haystack['Query']);
// -                continue;
// -            }
// -            else if( !is_int($K) )
// -            {
// -                URLEncoded::Set(array(array($K=>$V)),$Haystack['Query']);
// -                continue;
// -            }
// -
// -            $V2 = explode('#',$V);
// -            if( isset($V2[1]) )
// -                static::SetFragment($V2[1],$Haystack);
// -
// -            $V2 = explode('?',$V2[0]);
// -            if( isset($V2[1]) )
// -                URLEncoded::Set($V2[1],$Haystack['Query']);
// -
// -            if( !empty($V2[0]) )
// -            {
// -                if( ($V2[0][0] === '>' || $V2[0][0] === '<') )
// -                {
// -                    Path::Set($V2[0],$Haystack['Path']);
// -                    $Haystack['Path']['IsAbs'] = TRUE;
// -                }
// else
// -                {
// -                    trigger_error("URL::Set() Unrecognized change string '{$V}'");
// -                }
// -            }
// -        }
// -    }
// -

// * Add or remove key/values in a URLEncoded Struct.
// -     *
// -     * $Needle is change string, an array of change strings, or an array
// -     * of associative array key/value pairs, defining the changes to make:
// -     *  - @c ?key=value&key1=value1: set the key/value pairs from a string.  setting empty will delete the key.
// -     *  - @c ?: remove all key/values
// -     *  - @c array('key'=>'value'): set the key/value from an array
// -     *  - @c array('key'=>NULL): remove the key/value, if it exists
// -     *  - @c array(): remove all key/values
// -     *
// -     * @param array $Needle An array of key/values as strings or arrays.
// -     * @param string $Needle A key=value change string.
// -     * @param array &$Haystack URLEncoded Struct.
// -     * @retval void
// -     */
// -    public static function Set( $Needle,&$Haystack )
// -    {
// -        foreach( (array) $Needle as $K => $V )
// -        {
// -            if( empty($V) || $V === '?' )
// -            {
// -                $Haystack = array();
// -                continue;
// -            }
// -
// -            if( is_string($V) )
// -            {
// -                parse_str(ltrim($V,'?'),$V2);
// -            }
// -            else if( is_array($V) )
// -            {
// -                $V2 = $V;
// -            }

// -
// -            foreach( $V2 as $I => $J )
// -            {
// -                if( isset($Haystack[$I]) && empty($J) )
// -                    static::Del($I,$Haystack);
// -                else
// -                    $Haystack[$I] = $J;
// -            }
// -        }
// -    }
// -

// -    /**
// -     * Prepend or append a subdomain in a Hostname.
// -     *
// -     * $Needle is a string defining the change to make:
// -     *  - \c <subdomain: prepend the subdomain.
// -     *  - \c >subdomain: append the subdomain.
// -     *
// -     * @param string $Needle Direction and subdomain to add.
// -     * @param array $Haystack Hostname Struct.
// -     * @retval int The position the subdomain was added.
// -     * @retval NULL Operation not recognized.
// -     *
// -     * @todo This may be expanded slightly.
// -     */
// -    public static function Set( $Needle,&$Haystack )
// -    {
// -        $Needle = trim($Needle);
// -
// -        if( $Needle[0] === '<' )
// -            return static::Prepend(ltrim($Needle,'<'),$Haystack);
// -        else if( $Needle[0] === '>' )
// -            return static::Append(ltrim($Needle,'>'),$Haystack);
// -        else
// -            return NULL;
// -    }




// public static function Set( $Needle,&$Haystack )
// -    {
// -        foreach( (array) $Needle as $K => $V )
// -        {
// -            if( is_array($V) )
// -            {
// -                URLEncoded::Set($V,$Haystack['Query']);
// -                continue;
// -            }
// -            else if( !is_int($K) )
// -            {
// -                URLEncoded::Set(array(array($K=>$V)),$Haystack['Query']);
// -                continue;
// -            }
// -
// -            $V2 = explode('#',$V);
// -            if( isset($V2[1]) )
// -                static::SetFragment($V2[1],$Haystack);
// -
// -            $V2 = explode('?',$V2[0]);
// -            if( isset($V2[1]) )
// -                URLEncoded::Set($V2[1],$Haystack['Query']);
// -
// -            if( !empty($V2[0]) )
// -            {
// -                if( ($V2[0][0] === '>' || $V2[0][0] === '<') )
// -                {
// -                    Path::Set($V2[0],$Haystack['Path']);
// -                    $Haystack['Path']['IsAbs'] = TRUE;
// -                }
// -                else
// -                {
// -                    trigger_error("URL::Set() Unrecognized change string '{$V}'");

//  /**
// -     * Add or remove key/values in a URLEncoded Struct.
// -     *
// -     * $Needle is change string, an array of change strings, or an array
// -     * of associative array key/value pairs, defining the changes to make:
// -     *  - @c ?key=value&key1=value1: set the key/value pairs from a string.  setting empty will delete the key.
// -     *  - @c ?: remove all key/values
// -     *  - @c array('key'=>'value'): set the key/value from an array
// -     *  - @c array('key'=>NULL): remove the key/value, if it exists
// -     *  - @c array(): remove all key/values
// -     *
// -     * @param array $Needle An array of key/values as strings or arrays.
// -     * @param string $Needle A key=value change string.
// -     * @param array &$Haystack URLEncoded Struct.
// -     * @retval void
// -     */
// -    public static function Set( $Needle,&$Haystack )
// -    {
// -        foreach( (array) $Needle as $K => $V )
// -        {
// -            if( empty($V) || $V === '?' )
// -            {
// -                $Haystack = array();
// -                continue;
// -            }
// -
// -            if( is_string($V) )
// -            {
// -                parse_str(ltrim($V,'?'),$V2);
// -            }
// -            else if( is_array($V) )
// -            {
// -                $V2 = $V;

// -            foreach( $V2 as $I => $J )
// -            {
// -                if( isset($Haystack[$I]) && empty($J) )
// -                    static::Del($I,$Haystack);
// -                else
// -                    $Haystack[$I] = $J;
// -            }
// -        }
// -    }
// -