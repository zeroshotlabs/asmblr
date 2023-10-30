<?php declare(strict_types=1);
/**
 * @file types.php Base traits, interfaces and abstract classes.
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
 * 
 */
trait encoded_str
{
 //   public str_encodings $_encoding;


    /**
     * Create a new encoded string from an array.
     * 
     * @param array $arr Array of data, typically key/value pairs.
     * @return self A encoded string.
     */
    public static function arr( array $arr ): self
    {
        return new self($arr);       
    }

    // /**
    //  * Counts the number of key/value pairs.
    //  * 
    //  * @return int The number of key/value pairs.
    //  */
    // public function count(): int
    // {
    //     return count(array_keys($this));
    // }

    /**
     * Create a string from the data.
     *
     * @return string The URL encoded query string.
     *
     * @note This uses http_build_query() with $encoding which defaults to PHP_QUERY_RFC3986.
     * @note This uses arg_separator.output.
     * @note A '?' is automatically prefixed.    @todo  not done now
     */
    public function __toString(): string
    {
        return http_build_query($this->getArrayCopy(),'',null,$this->_encoding);
    }
}


/**
 * Enable classes to send and receive "tokens", or class-specific key/value pairs,
 * often representing configuration or containerized actions and parameters.
 * 
 * @see asm\types\token
 */
trait tokenized
{
    public array $tokens = [];
    public string|null $type = null;

    /**
     * Parses and initializes tokens for caching by config.
     * 
     * While a class can override this method to provide it's own parsing,
     * by default this parses as follows:
     *  - key=value&key1=value1 - set the key/value pairs
     *  - key:{op}:value - set the key/value pairs, performing an optional operation:
     *      - < > - prepend or append the value
     *      - = - set the value (generally same as append)
     *      - empty/null - remove the key/value pair
     * 
     * Most typically used for paths, titles, etc.
     * 
     * The value can also be a JSON string, in which case it's converted as a
     * key/value object. 
     *
     * @param array $tokens Array of raw tokens from config.
     * @return array Parsed tokens.
     * @throws e500 Unexpected token type.
     * @note While the creation of tokens are cached in config, they are applied
     *       at runtime in a class specific way.
     */
    public static function parse_tokens( array $tokens ): array
    {
        $parsed = [];
        foreach( $tokens as $k => $v )
        {
            if( !is_string($v) )
                throw new e500("Token not string for '$k'");

            if( isset($parsed[$k]) )
                throw new e500("Duplicate token '$k'");

            // json
            if( strpos($v,'{') !== false && strpos($v,'}') !== false )
                $parsed[$k] = json_decode($v,true);
            // internal change string
            else
                $parsed[$k] = array_filter(explode(':',$v));
        }
        
        return $parsed;
    }

    /**
     * Create a token to be used with use_token().
     * 
     * Generally this will create a token of the same type as the current
     * token type defined for the class, but doesn't have to.
     * 
     * Tokens are most typically created during configuration initialization.
     */
    public function create_token( $name,$type = '',... $args ): array
    {
        $t = $this->_generic;
        $t[0] = $name;
        $t[1] = $type??$this->type;
        $t[2] = $args;

        return $t;
    }
}

