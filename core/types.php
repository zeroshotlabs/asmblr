<?php
/**
 * @file Skel.php Base traits, interfaces and abstract classes..
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\types;

use Stringable;

interface endpointi
{
    
}


/**
 * Define the encoding types available.
 * 
 * @see \asm\Querystring
 */
enum encodings 
{
    /**
     * Encoded as application/x-www-form-urlencoded.
     * Spaces become +
     */
    case PHP_QUERY_RFC1738;

    /**
     * URL encoded.
     * Spaes become %20
     */
    case PHP_QUERY_RFC3986;
} 



/**
 * Directives allow keys/values to be pushed from the config into an object.
 */
interface directable
{
    /**
     * Apply a directive.
     *
     * @param string $key The key - or name - of the directive to set.
     * @param mixed $value The value of the directive.
     */
    public function apply_directive( $key,$value );
}


/*
 * Implement directable interface.
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
    public function apply_directive( $key,$value )
    {
        $this->directives[$key] = $value;
    }
}




// /**
//  * General purpose data access objects.
//  * 
//  * @todo supersede array based struct across the board (asmblr v6).
//  * @todo implement full features like debug/etc 
//  * @todo revisit dynamic properties (again)
//  */
// abstract class dao implements \ArrayAccess,Debuggable,Directable
// {
//     use Debugged;
//     use Directived;
//     use DAOCountableArray

// }



/**
 * Data Access Object
 * Flexible data object designed for key/value pairs and array access.
 * 
 * @implements \ArrayAccess
 * @implements \Countable
 * 
 * @note Not really suitable for tabular data (no iteration) and count()
 * returns the number of properties.
 */
trait countable_array
{
    /*
     * @implements \ArrayAccess
     */
    public function offsetGet( $key ): mixed
    {
        return isset($this->$key)?$this->$key:NULL;
    }
    public function offsetSet( $key,$value ): void
    {
        $this->$key = $value;
    }
    public function offsetExists( $key ): bool
    {
        return $this->__isset($key);        
    }
    public function offsetUnset( $key ): void
    {
        unset($this->$key);
    }

    /**
     * @implements \Countable
     * @return int The number of properties.
     */
    public function count(): int
    {
        return count(get_object_vars($this));
    }
}


/**
* Implement dynamic properties for key/value storage within a container.
* Key/values are stored in the kv property, not actually as dynamic properties.
*/
trait dynamic_kv
{
    protected $kv = [];

    public function __get( $key ): mixed
    {
        return isset($this->kv[$key])?$this->kv[$key]:NULL;
    }
    public function __set( $key,$value ): void
    {
        $this->kv[$key] = $value;
    }
    public function __isset( $key ): bool
    {
        return isset($this->kv[$key]);
    }
    public function __unset( $key ): void
    {
        $this->kv[$key] = NULL;
        unset($this->kv[$key]);
    }    
}

/**
 * @todo not implemented
 */
interface extension
{

}

// /**
//  * Interface for classes which wish to implement CREATE, READ, UPDATE,
//  * DELETE and COUNT functionality on a data source.
//  */
// interface CRUDC
// {
//     public function CREATE( $Table,$values );

//     public function READ( $Table,$Constraint = NULL,$Columns = NULL,$OrderBy = NULL );

//     public function UPDATE( $Table,$values,$Constraint );

//     public function DELETE( $Table,$Constraint );

//     public function COUNT( $Table,$Constraint = NULL );
// }


/**
 * Tools for manipulating RFC 1738/3986 query strings, such as for GET and some POST data.
 *
 * @note This should not be used for multipart/form-data (form uploads)).
 * @note This uses http_build_query() with PHP_QUERY_RFC3986 by default.
 * 
 * @todo There's no way to change the encoding except to extend.
 */
class encoded implements \stringable,\Countable
{
    use \asm\types\dynamic_kv;
    protected $encoding = encodings::PHP_QUERY_RFC3986;


    public function __construct( public readonly array $pairs )
    { }

    public function __get( string $Label ): mixed
    {
        return isset($this->pairs[$Label])?$this->pairs[$Label]:null;
    }

    public function __set( string $Label,mixed $Value ): void
    {
        $this->pairs[$Label] = $Value;
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
     * Create a new encoded from a string.
     *
     * @param string $str The encoded string to parse.
     * @return \asm\types\encoded A encoded.
     *
     * @note This uses parse_str().
     */
    public static function str( string $str ): encoded
    {
        $Q = [];
        parse_str($str,$Q);
        return new self($Q);
    }

    /**
     * Create a new encoded DAO from an array.
     * 
     * @param array $arr Array of data, typically key/value pairs.
     * @return \asm\encoded A encoded DAO.
     */
    public static function arr( array $arr ): encoded
    {
        return new self($arr);
    }

    /**
     * Create a string from the encoded.
     *
     * @return string The URL encoded query string.
     *
     * @note This uses http_build_query() with $Encoding which defaults to PHP_QUERY_RFC3986.
     * @note This uses arg_seperator.output.
     * @note A '?' is automatically prefixed.
     */
    public function __tostring(): string
    {
        if( !empty($this) )
            return '?'.http_build_query($this->pairs,'',null,$this->Encoding);
        else
            return '';
    }
}


/**
 * Tools for working with a UNIX or URL path.
 *
 * By default, a path uses the forward slash @c / as a seperator.  While the
 * back slash can be used, no handling of special Windows paths or drives is done.
 *
 * A Segment is what's contained between two seperators, or a seperator
 * and the end of the string, which implies a directory.
 *
 * @note No automatic encoding/decoding/escaping is done.
 * @note This is platform agnostic - it doesn't know if it's running under Windows or Unix.
 * @note This uses read-only and is not reusable.
 * @note This is not a security mechanism - use realpath().
 */
class path implements \stringable
{
    /**
     * Use path::path() or path::url() instead.
     */
    private function __construct(
        /**
         * The path's seperator.
         */
        public readonly string $seperator = '/',

        /**
         * TRUE if the path has a leading seperator.
         */
        public readonly bool $IsABS,

        /**
         * TRUE if the path has a trailing seperator.
         */
        public readonly bool $IsDir,

        /**
         * TRUE if the path is only the seperator (IsDir and IsAbs will also be TRUE).
         */
        public readonly bool $IsRoot,

        /**
         * TRUE if the path is a shell path, otherwise it's escaped as a URL path.
         */
        public readonly bool $IsShell,

        /**
         * Numeric array of the pieces between the seperators.
         */
        public readonly array $segments = []
    )
    { }

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
            return $this->seperator;
        }
        else
        {
            $shell = $IsShell===NULL?$this->IsShell:$IsShell;
            $abs = $IsABS===NULL?$this->IsABS:$IsABS;
            $dir = $IsDir===NULL?$this->IsDir:$IsDir;

            if( $shell === false )
                $Segs = implode($this->seperator,array_map('rawurldecode',$this->segments));
            else
                $Segs = implode($this->seperator,array_map('escapeshellcmd',$this->segments));

            return ($abs?$this->seperator:'').$Segs.($dir?$this->seperator:'');
        }
    }    
    /**
     * Create a filesystem path from a string.
     *
     * A backslash seperator is automatically detected if there is one, otherwise a forward
     * slash is the default.
     *
     * @param string $str The path string to parse, an empty string, or NULL.
     * @param string $seperator Specify a single character as a seperator to use.
     * @param bool $shell TRUE if the path is a shell path, otherwise it'll be escaped as a URL path.
     * @return \asm\types\path A path.
     *
     * @note An empty or NULL $str, or one that is only multiple seperators,
     * 		 will be considered a root path.
     * @note This passes through query strings but shouldn't be depended on.
     */
    public static function path( string $str,string $seperator = NULL,bool $shell = true ): path
    {
        if( !is_string($str) && !empty($str = trim($str)) )
        {
            _stde("path::path() - str is not a string (".gettype($str).")");
            return null;
        }

        if( empty($seperator) )
        {
            if( strpos($str,'\\') !== false )
                $seperator = '\\';
            else
                $seperator = '/';
        }

        $segments = [];
        $str = trim($str);

        // a root path
        if( empty($str) || $str === $seperator )
        {
            $segments[0] = $seperator;
            $IsAbs = $IsDir = $IsRoot = true;
        }
        else
        {
            $IsRoot = false;
            $IsAbs = $str[0]===$seperator?true:false;
            $IsDir = substr($str,-1,1)===$seperator?true:false;
            // $segments = preg_split("(\\{$seperator}+)",$str,-1,PREG_SPLIT_NO_EMPTY);
            // the fastest way according to chatgpt - after some persuading
            $segments = array_filter(explode($seperator,$str));
        }

        return new self($seperator,$IsAbs,$IsDir,$IsRoot,$shell,$segments);
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
class hostname implements \stringable
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
     * Overload to return the hostname as a string.
     */
    public function __tostring(): string
    {
        return implode('.',array_reverse($this->subdomains));
    }
}


/**
 * Interface for classes which wish to manipulate a set key/value pairs.
 */
// interface kvS extends \Iterator,\Countable,\ArrayAccess
// {
//     public function __get( $key );

//     public function __set( $key,$value );

//     public function __isset( $key );

//     public function __unset( $key );

//     public function Export();
// }





/**
 * Allow internal debugging of the implementing class.
 */
interface Debuggable
{
    /**
     * Turn on debugging for the Wire()'d object.
     *
     * Debugging behavior is determined only by the object itself and should
     * be on when a configured DebugToken is present in $_SERVER.
     */
    public function DebugOn( $Label = NULL );

    /**
     * Turn off debugging for the Wire()'d object.
    */
    public function DebugOff();

}

/**
 * Default debugging methods for the Debuggable interface.
 */
trait Debugged
{
    /**
     * Token for toggling debugging for the object.
     * It can be checked using @c isset($_SERVER[$this->DebugToken])
     */
    protected $DebugToken;


    /**
     * Enable debugging.
     *
     * Debugging is controlled by an element of name @c Debugged::$DebugToken
     * in the @c $_SERVER super-global.
     *
     * @param NULL $Label Log messages will be labeled with the class name.
     * @param string $Label Custom label for log messages.
     *
     * @todo $Label is misused in PageSet/TemplateSet as a way to trigger output of debug info - needs clean-up.
     */
    public function DebugOn( $Label = NULL )
    {
        if( empty($Label) )
            $this->DebugToken = get_class($this);
        else
            $this->DebugToken = $Label;

        $_SERVER[$this->DebugToken] = TRUE;
    }

    /**
     * Disable debugging.
     */
    public function DebugOff()
    {
        if( !empty($this->DebugToken) )
            unset($_SERVER[$this->DebugToken]);
    }

    /**
     * Determine whether debugging is enabled.
     *
     * @retval boolean TRUE if debugging is enabled.
     */
    public function IsDebug()
    {
        return !empty($_SERVER[$this->DebugToken]);
    }
}


////// to be considered

    // public function use( object $DAO ): void
    // {
    //     $this->kv = $DAO->kv;
    // }

//    abstract public function __construct( );

    /**
     * Get a column of values from a Is::Columnar() array.
     * 
     * array_column
     * 
     * 
     * @param string $Needle The name of the column to get.
     * @param array $Haystack A columnar array.
     * @retval array The column's values.
     * @retval NULL The array isn't columnar.
     *
     * @note If some "rows" don't have the column specified, the NULL value will be filled in.
     */
    // public static function GetColumn( $Needle,$Haystack )
    // {
    //     if( Is::Columnar($Haystack) === TRUE )
    //     {
    //         $Column = array();
    //         foreach( $Haystack as $V )
    //             $Column[] = (isset($V[$Needle])===TRUE?$V[$Needle]:NULL);
    //         return $Column;
    //     }
    //     else
    //         return NULL;
    // }

    
//     /**
//      * Convert a string, or recursively convert an array, to UTF-8.
//      *
//      * @param array,string $A A reference to the array to convert.
//      *                        A reference to the string to convert.
//      *
//      * @todo Full testing when fed strangely encoded strings.
//      * @note Changes original value.
//      */
//     public static function ToUTF8( &$A ): void
//     {
//         if( is_array($A) === TRUE )
//         {
//             foreach( $A as &$V )
//             {
//                 // Avoid encoding already encoded UTF-8 - TRUE is required to make a strict test.
//                 if( is_string($V) === TRUE && mb_detect_encoding($V,'UTF-8, ISO-8859-1',TRUE) !== 'UTF-8')
//                     $V = mb_convert_encoding($V,'UTF-8');
//                 else if( is_array($V) === TRUE )
//                     static::ToUTF8($V);
//             }
//         }
//         else if( is_string($A) === TRUE )
//         {
//             if( mb_detect_encoding($A,'UTF-8, ISO-8859-1',TRUE) !== 'UTF-8')
//                 $A = mb_convert_encoding($A,'UTF-8');
//         }
//     }
// }



    // /**
    //  * @note Implement SPL's Iterator interface.
    //  * @note This is where KeyValueSet::$kvPosition and KeyValueSet::$kvLength are initialized.
    //  */
    // public function rewind(): void
    // {
    //     $this->kvLength = count($this->kv);
    //     $this->kvPosition = 0;
    //     reset($this->kv);
    // }

    // /**
    //  * @retval mixed
    //  * @note Implement SPL's Iterator interface.
    //  * @note Use __get() in extending classes to maintain the processing behavior it does.
    //  */
    // public function current()
    // {
    //     return $this->__get(key($this->kv));
    // }

    // /**
    //  * @retval mixed
    //  * @note Implement SPL's Iterator interface.
    //  */
    // public function key()
    // {
    //     return key($this->kv);
    // }

    // /**
    //  * @note Implement SPL's Iterator interface.
    //  * @note Uses {@link $kvPosition}.
    //  */
    // public function next(): void
    // {
    //     ++$this->kvPosition;
    //     next($this->kv);
    // }

    // /**
    //  * @retval boolean
    //  * @note Implement SPL's Iterator interface.
    //  * @note Uses {@link $kvPosition} and {@link $kvLength}.
    //  */
    // public function valid(): bool
    // {
    //     return ($this->kvPosition < $this->kvLength);
    // }




    // /**
    //  * @retval mixed
    //  * @note Implement SPL's ArrayAccess interface.
    //  */
    // public function offsetGet( $Key )
    // {
    //     return $this->__get($Key);
    // }

    // /**
    //  * @note Implement SPL's ArrayAccess interface.
    //  */
    // public function offsetSet( $Key,$Value ): void
    // {
    //     $this->__set($Key,$Value);
    // }

    // /**
    //  * @retval boolean
    //  * @note Implement SPL's ArrayAccess interface.
    //  */
    // public function offsetExists( $Key ): bool
    // {
    //     return $this->__isset($Key);
    // }

    // /**
    //  * @note Implement SPL's ArrayAccess interface.
    //  */
    // public function offsetUnset( $Key ): void
    // {
    //     $this->__unset($Key);
    // }

