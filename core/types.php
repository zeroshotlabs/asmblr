<?php declare(strict_types=1);
/**
 * @file types.php Base traits, interfaces and abstract classes.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\types;
use function asm\sys\_stde;
use asm\_e\e500;


/**
 * General purpose data access objects.
 * 
 * Used internally to read config settings but generally useful.
 * 
 * @implements \ArrayAccess
 * @implements \Countable
 * @implements \Iterator
 */
class dao extends \ArrayObject
{
// implements \ArrayAccess,\Countable,\Iterator
// {
//     use dynamic_kv;
//     use iterable_kv;
//     use object_array;

    /**
     * Instantiates a dao wrapped around an array.
     * 
     * Doesn't reference the original array.
     */
    public function __construct( array $kv,$flags = \ArrayObject::ARRAY_AS_PROPS )
    {
        parent::__construct($kv,$flags);
    }

    public function __clone()
    {
        $this->exchangeArray($this->getArrayCopy());
    }
}


/**
 * Define the encoding types available.
 * 
 * @see \asm\types\encoded_str
 */
enum encodings: int
{
    /**
     * Encoded as application/x-www-form-urlencoded (spaces become +).
     */
    case PHP_QUERY_RFC1738 = \PHP_QUERY_RFC1738;

    /**
     * URL encoded (spaces become %20)
     */
    case PHP_QUERY_RFC3986 = \PHP_QUERY_RFC1738;
}


/**
 * Tools for manipulating RFC 1738/3986 query strings, such as for GET and some POST data.
 *
 * @note This should not be used for multipart/form-data (form uploads)).
 * @note This uses http_build_query() with PHP_QUERY_RFC3986 by default.
 * 
 * @todo https://www.php.net/manual/en/ref.url.php   ?
 * @todo should be cleaned up, more aligned with dao
 */
class encoded_str extends dao implements \Stringable
// implements \stringable,\Countable
{
//    use \asm\types\dynamic_kv;
    protected $encoding = encodings::PHP_QUERY_RFC3986->value;     // PHP_QUERY_RFC1738


    public function __construct( public readonly array $pairs,encodings $encoding = null )
    {
        $this->encoding = $encoding->value ?? $this->encoding;
    }

    public function __get( string $label ): mixed
    {
        return isset($this->pairs[$label])?$this->pairs[$label]:null;
    }

    /**
     * @todo probably won't work because of readonly $pairs
     */
    public function __set( string $label,mixed $value ): void
    {
        $this->pairs[$label] = $value;
    }


    /**
     * Create a new encoded from a string.
     *
     * @param string $str The encoded string to parse.
     * @return \asm\types\encoded_str A encoded string.
     *
     * @note This uses parse_str().
     */
    public static function str( string $str ): encoded_str
    {
        $Q = [];
        parse_str($str,$Q);
        return new self($Q);
    }


    /**
     * Create a new encoded string from an array.
     * 
     * @param array $arr Array of data, typically key/value pairs.
     * @return \asm\types\encoded_str A encoded string.
     */
    public static function arr( array $arr ): encoded_str
    {
        return new self($arr);       
    }


    /**
     * Counts the number of key/value pairs.
     * 
     * @return int The number of key/value pairs.
     */
    public function count(): int
    {
        return count($this->pairs);
    }


    /**
     * Create a string from the encoded.
     *
     * @return string The URL encoded query string.
     *
     * @note This uses http_build_query() with $Encoding which defaults to PHP_QUERY_RFC3986.
     * @note This uses arg_separator.output.
     * @note A '?' is automatically prefixed.
     */
    public function __toString(): string
    {
        if( !empty($this->pairs) )
            return '?'.http_build_query($this->pairs,'',null,$this->encoding);
        else
            return '';
    }
}



/**
 * @todo not implemented
 */
interface extension
{

}



/**
 * Directives allow keys/values to be pushed from the config into an object.
 * 
 * @todo Not implemented; maybe done differently now?
 */
interface directable
{
    /**
     * Apply a directive.
     *
     * @param string $key The key - or name - of the directive to set.
     * @param mixed $value The value of the directive.
     */
    public function apply( $key,$value );
}

/*
 * Implement directable interface.
 * 
 * @todo Not implemented; maybe done differently now?
 */
trait directed
{
    /**
     * @property array $Directives An associative array of key/value pairs set by Config.
     */
    protected $directives = [];

    /**
     * Set key/value pairs from Config directives in $Directives property.
     *
     * @param string $key The name of the key to set.
     * @param mixed $value The value to set.
     */
    public function apply( $key,$value )
    {
        $this->directives[$key] = $value;
    }
}


/**
 * Implement dynamic properties for key/value storage within a container.
 * Key/values are stored in the kv property, not actually as dynamic properties.
 *
 * @note there is no column or iterator support currently.
 * @note should do a array_column/array_is_list implementation
//  */
// trait dynamic_kv
// {
//     protected $kv = [];

//     public function __clone()
//     {
//         $this->kv = $this->kv;
//     }

//     /**
//      * Changes the internal element to use for keys/values.
//      * 
//      * By reference thus changes the original.
//      */
//     public function use_kv( string $name )
//     {
//         $this->kv = &$this->{$name};
//     }
    
//     public function __get( $key ): mixed
//     {
//         return isset($this->kv[$key])?$this->kv[$key]:NULL;
//     }
//     public function __set( $key,$value ): void
//     {
//         $this->kv[$key] = $value;
//     }
//     public function __isset( $key ): bool
//     {
//         return isset($this->kv[$key]);
//     }
//     public function __unset( $key ): void
//     {
//         $this->kv[$key] = NULL;
//         unset($this->kv[$key]);
//     }    
// }

// /**
//  * Provides iteration over the key/value pairs, $kv.
//  * 
//  * @implements Iterator
//  * @implements Countable
//  */
// trait iterable_kv22
// {
//     protected $kv = [];
//     protected $_len = 0;
//     protected $_posi = 0;

//     // use dynamic_kv {
//     //     dynamic_kv::use_kv as use_kv;
//     // }

//     public function count(): int
//     {
//         return count($this->kv);
//     }

//     /**
//      * Also initializes the length and position.
//      */
//     public function rewind(): void
//     {
//         $this->_len = count($this->kv);
//         $this->_posi = 0;
//         reset($this->kv);
//     }

//     public function current(): mixed
//     {
//         return $this->kv[key($this->kv)];
//     }

//     public function key(): string|int
//     {
//         return key($this->kv);
//     }

//     public function next(): void
//     {
//         ++$this->_posi;
//         next($this->kv);
//     }

//     public function valid(): bool
//     {
//         return ($this->_posi < $this->_len);
//     }
// }


// /**
//  * Flexible data object designed for key/value pairs and array access.
//  * 
//  * @implements \ArrayAccess
//  * 
//  * @note Not really suitable for tabular data (no iteration) and count()
//  * returns the number of properties.
//  * 
//  * @todo is this incompatible with dynamic_kv?
//  * @todo what aboutvuse of ArrayObject?
//  */
// trait object_array22
// {
//     public function offsetGet( $key ): mixed
//     {
//         return isset($this->$key)?$this->$key:NULL;
//     }
//     public function offsetSet( $key,$value ): void
//     {
//         $this->$key = $value;
//     }
//     // @note here __isset is used though above __get/__set isn't
//     public function offsetExists( $key ): bool
//     {
//         return $this->__isset($key);        
//     }
//     public function offsetUnset( $key ): void
//     {
//         unset($this->$key);
//     }
// }


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
//,\Iterator,\ArrayAccess,\Countable
{
    // use object_array;
    // use iterable_kv;

//     public function __clone()
//     {
// //        debug_print_backtrace();

//         $t = $this->kv;
//         unset($this->kv);
//         $this->kv = $t;
//     }
    /**
     * Use path::path() or path::url() instead.
     */
    private function __construct(
        /**
         * The path's separator.
         */
        public string $separator = '/',

        /**
         * TRUE if the path has a leading separator.
         */
        public bool $IsABS,

        /**
         * TRUE if the path has a trailing separator.
         */
        public bool $IsDir,

        /**
         * TRUE if the path is only the separator (IsDir and IsAbs will also be TRUE).
         */
        public bool $IsRoot,

        /**
         * TRUE if the path is a shell path, otherwise it's escaped as a URL path.
         */
        public bool $IsShell,

        /**
         * Numeric array of the pieces between the separators.
         */
        public array $segments = []
    )
    {
//        $this->use_kv('segments');
    }

    public function as_abs(): string
    {
        return $this->the_real_tostring(NULL,NULL,TRUE);
    }

    public function as_dir(): string
    {
        return $this->the_real_tostring(NULL,TRUE,NULL);
    }

    public function as_abs_dir(): string
    {
        return $this->the_real_tostring(NULL,TRUE,TRUE);
    }

    /**
     * Prepend segments of another path.
     * 
     * @param path $p The path to prepend.
     * @param bool $dedupe TRUE to remove duplicates.
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
     * @param bool $dedupe TRUE to remove duplicates.
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
     * on IsDir.
     */
    public function ordered( $inc = true ): array
    {
        if( $this->IsRoot )
            return [$this->separator];

        $p = [];
        foreach( $this->segments as $k => $v )
            $p[] = ($k>0?$p[$k-1]:(($this->separator))).$v.$this->separator;            

        if( !$this->IsDir )
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
     * according to IsShell, IsABS and IsDIR.
     * 
     * @todo PHP feature request: using python's slicing, printf formatting, etc?  Allow params?
     * @see the_real_tostring() for actual string creation and overrides.
     */
    public function __toString(): string
    {
        return $this->the_real_tostring();
    }

    protected function the_real_tostring( bool $IsShell = NULL,bool $IsDir = NULL,bool $IsABS = NULL ): string
    {
        if( $this->IsRoot === true )
        {
            return $this->separator;
        }
        else
        {
            $shell = $IsShell===NULL?$this->IsShell:$IsShell;
            $abs = $IsABS===NULL?$this->IsABS:$IsABS;
            $dir = $IsDir===NULL?$this->IsDir:$IsDir;

            if( $shell === false )
                $Segs = implode($this->separator,array_map('rawurldecode',$this->segments));
            else
                $Segs = implode($this->separator,array_map('escapeshellcmd',$this->segments));

            return ($abs?$this->separator:'').$Segs.($dir?$this->separator:'');
        }
    }
    

    /**
     * Create a filesystem path from a string.
     *
     * A backslash separator is automatically detected if there is one, otherwise a forward
     * slash is the default.
     *
     * @param string $str The path string to parse, an empty string, or NULL.
     * @param string $separator Specify a single character as a separator to use.
     * @param bool $shell TRUE if the path is a shell path, otherwise it'll be escaped as a URL path.
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
            $IsAbs = $IsDir = $IsRoot = true;
        }
        else
        {
            $IsRoot = false;
            $IsAbs = $str[0]===$separator?true:false;
            $IsDir = substr($str,-1,1)===$separator?true:false;
            // $segments = preg_split("(\\{$separator}+)",$str,-1,PREG_SPLIT_NO_EMPTY);
            // the fastest way according to chatgpt - after some guidance
            // the double reverse is to reindex (not from chatgpt :)
            $segments = array_reverse(array_reverse(array_filter(explode($separator,$str))));
        }

        return new self($separator,$IsAbs,$IsDir,$IsRoot,$shell,$segments);
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
 * @note This does not support full URL/URI, username, port, etc. pedantics and basically straightforward.
 * @note If port is specified as 80 or 443, it'll be set as an empty string and not included when output as a string.
 */
class url implements \Stringable
{
    /**
     * TRUE if the scheme is https.
     */
    public bool $IsHTTPS;

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
         * 
         * @note these should be "readonly" though it is handy to change (carefully) and re-string
         */
        public string $scheme,

        public string $username,

        public string $password,

        public \asm\types\hostname $hostname,

        public string $port,

        public \asm\types\path $path,

        public \asm\types\encoded_str $encoded,

        public string $fragment
    )
    {
        $this->IsHTTPS = ($scheme === 'https');
        $this->port = ($port === '80' || $port === '443' ? '' : $port);
    }

    public function __clone()
    {
        $this->hostname = clone $this->hostname;
        $this->path = clone $this->path;
        $this->encoded = clone $this->encoded;
    }

    /**
     * Create a new URL from a string.
     *
     * This uses parse_url() and follows it's semantics, whereby some URL strings will be indeterminate.
     * 
     * @param string $url_str The URL string to parse.
     * @throws Exception Malformed URL '$url_str' (parse_url()).
     * @return \asm\types\url A URL.
     *
     * @note This uses parse_url() and defaults to https if not specified. It attempts
     * to detect and auto-correct malformed URLs but it's not perfect; filename.html is
     * parsed as domain.com for example.
     */
    public static function str( $url_str ): \asm\types\url|null
    {
        if( !is_string($url_str) || empty($url_str = trim($url_str)) )
        {
            _stde("URL::str() - url_str is not a string (".gettype($url_str).")");
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

        $encoded = encoded_str::str($T['encoded'] ?? '');
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
            if( $this->port && ($this->port != '80' && $this->port != '443') )
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

        // @todo confirm that this correctly returns partial URLs, like a path only, etc. per below
        return "{$host}{$path}{$encoded}{$frag}";

        // // if a hostname is present, ensure a / for relative paths - otherwise use what path has
        // $Str .= (!empty($Str)&&empty($URL['path']['IsAbs'])?'/':'').path::ToString($URL['path'],'url');
        // $Str .= empty($URL['encoded'])?'':URLEncoded::ToString($URL['encoded']);
        // $Str .= $URL['fragment']===''?'':'#'.rawurlencode($URL['fragment']);
    }
}


