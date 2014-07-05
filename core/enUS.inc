<?php
/**
 * @file enUS.inc Default en-US locale validation, text utilities, and HTML templating.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Tools for working with US English text, delimited strings (tags), and forms.
 *
 * enUS methods fall into the following categories:
 *  - String: manipulate strings of US English (latin) text.
 *  - Element: normalize and validate elements of an array.
 *
 * The element category of methods are typically used for validating forms using the
 * $_POST and $_GET superglobals.  For these methods, the following semantics apply:
 *  - the original source array is changed as part of their normalization.
 *  - NEEDS REVIEW: a form field is considered "empty" when it's trimmed value is the empty string ($var === '') or an array
 *    with zero elements (count($var) === 0)
 *
 * These methods are not character set/encoding aware and use standard PHP
 * string functions.  It may also be worth setting your locale explicity:
 *
 * setlocale(LC_ALL,'');  // Windows - use Control Panel settings
 * setlocale(LC_ALL,'en_US'); // Unix
 *
 * @see enUSHTMLSet for English (latin) HTML templating and other HTML tools.
 *
 * @todo setlocale() and related needs to be correctly implemented and documented.
 * @todo Possibly add countries, date, time, AM/PM, and more... and extract to additional classes.
 *       http://komunitasweb.com/2009/03/10-practical-php-regular-expression-recipes/
 */
abstract class enUS
{
    protected static $Plural = array(
        '/(quiz)$/i'               => "$1zes",
        '/^(ox)$/i'                => "$1en",
        '/([m|l])ouse$/i'          => "$1ice",
        '/(matr|vert|ind)ix|ex$/i' => "$1ices",
        '/(x|ch|ss|sh)$/i'         => "$1es",
        '/([^aeiouy]|qu)y$/i'      => "$1ies",
        '/(hive)$/i'               => "$1s",
        '/(?:([^f])fe|([lr])f)$/i' => "$1$2ves",
        '/(shea|lea|loa|thie)f$/i' => "$1ves",
        '/sis$/i'                  => "ses",
        '/([ti])um$/i'             => "$1a",
        '/(tomat|potat|ech|her|vet)o$/i'=> "$1oes",
        '/(bu)s$/i'                => "$1ses",
        '/(alias)$/i'              => "$1es",
        '/(octop)us$/i'            => "$1i",
        '/(ax|test)is$/i'          => "$1es",
        '/(us)$/i'                 => "$1es",
        '/s$/i'                    => "s",
        '/$/'                      => "s"
    );

    protected static $Singular = array(
        '/(quiz)zes$/i'             => "$1",
        '/(matr)ices$/i'            => "$1ix",
        '/(vert|ind)ices$/i'        => "$1ex",
        '/^(ox)en$/i'               => "$1",
        '/(alias)es$/i'             => "$1",
        '/(octop|vir)i$/i'          => "$1us",
        '/(cris|ax|test)es$/i'      => "$1is",
        '/(shoe)s$/i'               => "$1",
        '/(o)es$/i'                 => "$1",
        '/(bus)es$/i'               => "$1",
        '/([m|l])ice$/i'            => "$1ouse",
        '/(x|ch|ss|sh)es$/i'        => "$1",
        '/(m)ovies$/i'              => "$1ovie",
        '/(s)eries$/i'              => "$1eries",
        '/([^aeiouy]|qu)ies$/i'     => "$1y",
        '/([lr])ves$/i'             => "$1f",
        '/(tive)s$/i'               => "$1",
        '/(hive)s$/i'               => "$1",
        '/(li|wi|kni)ves$/i'        => "$1fe",
        '/(shea|loa|lea|thie)ves$/i'=> "$1f",
        '/(^analy)ses$/i'           => "$1sis",
        '/((a)naly|(b)a|(d)iagno|(p)arenthe|(p)rogno|(s)ynop|(t)he)ses$/i'  => "$1$2sis",
        '/([ti])a$/i'               => "$1um",
        '/(n)ews$/i'                => "$1ews",
        '/(h|bl)ouses$/i'           => "$1ouse",
        '/(corpse)s$/i'             => "$1",
        '/(us)es$/i'                => "$1",
        '/s$/i'                     => ""
    );

    protected static $Irregular = array('move'=>'moves','foot'=>'feet','goose'=>'geese','sex'=>'sexes',
                                        'child'=>'children','man'=>'men','tooth'=>'teeth','person'=>'people');

    protected static $Uncountable = array('sheep','fish','deer','series','species','money','rice','information','equipment');

    protected static $States = array('AL'=>'Alabama','AK'=>'Alaska','AS'=>'American Samoa','AZ'=>'Arizona','AR'=>'Arkansas',
                                     'CA'=>'California','CO'=>'Colorado','CT'=>'Connecticut','DE'=>'Delaware','DC'=>'District Of Columbia',
                                     'FL'=>'Florida','GA'=>'Georgia','GU'=>'Guam','HI'=>'Hawaii','ID'=>'Idaho','IL'=>'Illinois',
                                     'IN'=>'Indiana','IA'=>'Iowa','KS'=>'Kansas','KY'=>'Kentucky','LA'=>'Louisiana','ME'=>'Maine',
                                     'MH'=>'Marshall Islands','MD'=>'Maryland','MA'=>'Massachusetts','MI'=>'Michigan','MN'=>'Minnesota',
                                     'MS'=>'Mississippi','MO'=>'Missouri','MT'=>'Montana','NE'=>'Nebraska','NV'=>'Nevada',
                                     'NH'=>'New Hampshire','NJ'=>'New Jersey','NM'=>'New Mexico','NY'=>'New York','NC'=>'North Carolina',
                                     'ND'=>'North Dakota','MP'=>'Northern Mariana Islands','OH'=>'Ohio','OK'=>'Oklahoma','OR'=>'Oregon',
                                     'PW'=>'Palau','PA'=>'Pennsylvania','PR'=>'Puerto Rico','RI'=>'Rhode Island','SC'=>'South Carolina',
                                     'SD'=>'South Dakota','TN'=>'Tennessee','TX'=>'Texas','UT'=>'Utah','VT'=>'Vermont',
                                     'VI'=>'Virgin Islands','VA'=>'Virginia','WA'=>'Washington','WV'=>'West Virginia','WI'=>'Wisconsin',
                                     'WY'=>'Wyoming');

    protected static $Prefixes = array('Mr.','Mrs.','Ms.','Miss.','Master.','Prof.','Dr.');
    protected static $Suffixes = array('Sr.','Jr.','Ph.D.','M.D.','B.A.','M.A.','D.D.S.');

    protected static $UseStopWords = '/(\b(on|the|with|of|and|for|a|to|at|or|out|as|thy|from|how)\b)|(\40|\/|\.|@)/i';
    protected static $NoUseStopWords = '/(\40|\/|\.|@)/i';

    protected static $NoAlphaNum = '([^a-zA-Z0-9-])';
//    protected static $StopWords2 = '(-{2,})';
    protected static $StopWordsReplaceWith = array('-','','-');

    protected static $AllowStopWords = array(array('/(\40|\/)/i','([^a-zA-Z0-9-])','(-{2,})'),array('-','','-'));


    /**
     * Convert a string into an array of words.
     *
     * By default, the following is performed:
     *  - Stop words are replaced with dashes.
     *  - Non-alphanumeric characters are removed.
     *  - The string is split on dashes and those characters contained in $Delims.
     *  - Duplicate words are filtered.
     *
     * @param string $Str A string to convert.
     * @param boolean $RemoveStopWords TRUE to remove stop words.
     * @param boolean $DeDupe TRUE to remove duplicate words.
     * @param string $Delims A string of characters, in addition to the dash, to use as delimiters.
     */
    public static function ToWords( $Str,$RemoveStopWords = TRUE,$DeDupe = TRUE,$Delims = ',;:|' )
    {
        if( $RemoveStopWords === TRUE )
            $Str = preg_replace(array(static::$UseStopWords,static::$NoAlphaNum),array('-','-'),$Str);
        else
            $Str = preg_replace(array(static::$NoUseStopWords,static::$NoAlphaNum),array('-','-'),$Str);

        // do we want a space as a delim? do we need to trim each word?
        $Words = preg_split("/[ -{$Delims}]+/",$Str,PREG_SPLIT_NO_EMPTY);

        if( $DeDupe === TRUE )
            return array_unique($Words);
        else
            return $Words;
    }

    /**
     * Determine the boolean value of a string.
     *
     * The following is performed:
     *  - Strings of 'false', 'n', 'no' or '0' are considered to be boolean FALSE.
     *  - Strings of 'true', 'y', 'yes' or '1' are considered to be boolean TRUE.
     *
     * @param string $Str The string to convert.
     * @retval boolean TRUE or FALSE depending on the value of the string.
     * @retval NULL The boolean equivalent could not be determined.
     */
    public static function ToBoolean( $Str )
    {
        $Str = strtolower(trim($Str));
        if( in_array($Str,array('false','n','no','0'),TRUE) )
            return FALSE;
        else if( in_array($Str,array('true','y','yes','1'),TRUE) )
            return TRUE;
        else
            return NULL;
    }

    /**
     * Remove or replace smart characters with their ASCII equivalent.
     *
     * Smart characters are extended ASCII characters, typically found
     * in text copied from Microsoft Word documents.
     *
     * @param string $Src The string to clean.
     * @retval string The cleaned string.
     */
    public static function SmartStrip( $Src )
    {
        static $From = NULL;
        static $To = array('\'','\'','"','"','-','-','...',' ','e');

        if( $From === NULL )
            $From = array(chr(0x91),chr(0x92),chr(0x93),chr(0x94),
                          chr(0x96),chr(0x97),chr(0x85),chr(0xa0),chr(0xc3).chr(0xa9));

        return preg_replace("/([\x80-\xFF])/",'',str_replace($From,$To,$Src));
    }

    /**
     * Convert a word to it's plural form.
     *
     * @param string $S The word to convert.
     * @retval string The plural form of the word, or the same word if it couldn't be determined.
     *
     * @todo Confirm this works.
     *       http://kuwamoto.org/2007/12/17/improved-pluralizing-in-php-actionscript-and-ror/
     */
    public static function ToPlural( $S )
    {
        // save some time in the case that singular and plural are the same
        if( in_array(strtolower($S),self::$Uncountable) )
            return $S;

        // check for irregular singular forms
        foreach( self::$Irregular as $P => $R )
        {
            $P = '/'.$P.'$/i';

            if( preg_match($P,$S) )
                return preg_replace($P,$R,$S);
        }

        // check for matches using regular expressions
        foreach( self::$Plural as $P => $R )
        {
            if( preg_match($P,$S) )
                return preg_replace($P,$R,$S);
        }

        return $S;
    }

    /**
     * Convert a word to it's singular form.
     *
     * @param string $S The word to convert.
     * @retval string The singular form of the word, or the same word if it couldn't be determined.
     *
     * @todo Confirm this works.
     *       http://kuwamoto.org/2007/12/17/improved-pluralizing-in-php-actionscript-and-ror/
     */
    public static function ToSingular( $S )
    {
        // save some time in the case that singular and plural are the same
        if( in_array(strtolower($S),self::$Uncountable) )
            return $S;

        // check for irregular plural forms
        foreach ( self::$Irregular as $R => $P )
        {
            $P = '/' . $P . '$/i';

            if( preg_match($P,$S) )
                return preg_replace($P,$R,$S);
        }

        // check for matches using regular expressions
        foreach ( self::$Singular as $P => $R )
        {
            if( preg_match($P,$S) )
                return preg_replace($P,$R,$S);
        }

        return $S;
    }

    /**
     * Convert a count and item to a quantitative phrase.
     *
     * @param int $Count The number of items.
     * @param string $Item The name of the item.
     * @retval string A phrase of the form "$Count $Item" or "1 $Item"
     */
    public static function PluralizeIf( $Count,$Item )
    {
        if( ((int)$Count) === 1 )
            return "1 $Item";
        else
            return $Count.' '.self::ToPlural($Item);
    }

    /**
     * Create a blurb from a plain-text string.
     *
     * Any pre-processing, such as stripping HTML tags, should be done beforehand.
     *
     * @param string $Src String to create blurb from.
     * @param integer $LimitLen The maximum length of the blurb.
     * @param string $Ellipsis A string to append at the end of the blurb if the blurb is shorter than the source string.
     * @param boolean $BreakWord TRUE to allowing breaking within a word.
     * @param boolean $AlwaysEllipsis Set to TRUE to always include $Ellipsis, even if the text was under $LimitLen.
     * @retval string The blurb.
     */
    public static function ToBlurb( $Src,$LimitLen = 350,$Ellipsis = '...',$BreakWord = TRUE,$AlwaysEllipsis = FALSE )
    {
        if( $LimitLen >= ($StrLen = strlen($Src)) )
            return ($AlwaysEllipsis===TRUE?$Src.$Ellipsis:$Src);

        if( ($Src[$LimitLen-1] === ' ') || ($BreakWord === TRUE) )
        {
            return trim(substr($Src,0,$LimitLen)).$Ellipsis;
        }
        else
        {
            while( --$LimitLen > 0 )
                if( $Src[$LimitLen] === ' ' )
                    return trim(substr($Src,0,$LimitLen)).$Ellipsis;

            return '';
        }
    }

    /**
     * Create an SEO-friendly URL from a string.
     *
     * Returned URL will contain only words, with other characters and stopwords removed.
     * If provided, Token will be prefixed with an underscore.
     *
     * @param string $T String to create URL from.
     * @param string $Token Optional token prefix.
     * @retval string Cleaned URL encoded string..
     */
    public static function ToURL( $T,$Token = '' )
    {
        $Tmp = implode('-',self::ToWords($T));

        return urlencode(empty($Token)?$Tmp:"{$Token}_{$Tmp}");
    }

    /**
     * Validate whether a text element is of a certain value.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param string $Value The value that must exist.
     * @retval boolean TRUE if the value exists.
     *
     * @note This can be used to determine whether a form has been submitted.
     */
    public static function IsSubmit( $Label,$Src,$Value = 'Submit' )
    {
        return Struct::Get($Label,$Src,$Value);
    }

    /**
     * Normalize and validate a text element.
     *
     * The following is performed:
     *  - trim() and enUS::SmartStrip().
     *  - The length must be under 512 characters.
     *  - All characters must be printable and not newlines.
     *  - A scalar will be cast to a string; a non-scalar isn't valid.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @note This is the base for most other validating methods, thus everything is cast to a string.
     * @note An empty string or a non-isset() element are considered "empty".
     * @todo We may want a Is::Empty() or Is::Exists() or similar method since the isset/is_scalar dance is used often.
     */
    public static function Text( $Label,&$Src,$Required = TRUE )
    {
        if( isset($Src[$Label]) )
        {
            if( is_scalar($Src[$Label]) === FALSE )
                return NULL;

            $Src[$Label] = (string) $Src[$Label];
        }
        else
            return !$Required;

        $Src[$Label] = trim(self::SmartStrip($Src[$Label]));

        if( $Src[$Label] === '' )
            return !$Required;

        return ((strlen($Src[$Label])<512) && (ctype_print($Src[$Label])))?TRUE:NULL;
    }

    /**
     * Normalize and validate two text elements.
     *
     * The following is performed:
     *  - enUS:Text() to normalize and validate the two elements.
     *  - The value of the two elements must be identical (compared as strings).
     *
     * @param string $Label1 The name of the primary element to check.
     * @param string $Label2 The name of the secondary element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if both elements are allowed to be empty (or not exist).
     * @retval boolean TRUE if the elements exist and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate or the two elements were not equal.
     */
    public static function TextConfirm( $Label1,$Label2,&$Src,$Required = TRUE )
    {
        $R1 = static::Text($Label1,$Src,$Required);

        if( $R1 !== TRUE )
        {
            $Src[$Label2] = $Src[$Label1];
            return $R1;
        }

        if( empty($Src[$Label1]) )
        {
            $Src[$Label2] = '';
            return !$Required;
        }

        static::Text($Label2,$Src);

        return $Src[$Label1]===$Src[$Label2]?TRUE:NULL;
    }

    /**
     * Normalize and validate a text element as containing all digits.
     *
     * The following is performed:
     *  - enUS:Text() to normalize and validate the element.
     *  - ctype_digit() is used to ensure all digits.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @note This validates a string of digits ONLY - not an integer string or PHP integer type.  This means 0111 is valid
     * but 2.00 is NOT.
     */
    public static function Number( $Label,&$Src,$Required = TRUE )
    {
        $R = static::Text($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

//         if( empty($Src[$Label])  )
//             return !$Required;

        return ctype_digit($Src[$Label])?TRUE:NULL;
    }

    /**
     * Normalize and validate a textarea element.
     *
     * The following is performed:
     *  - trim() and enUS::SmartStrip().
     *  - All characters must be printable or \\t, \\r, \\n.
     *  - A scalar will be cast to a string; a non-scalar isn't valid.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @note The validation is effectively the same as Text() without the length check
     *       and allowance of \\t, \\r and \\t.
     */
    public static function Textarea( $Label,&$Src,$Required = TRUE )
    {
        if( isset($Src[$Label]) )
        {
            if( is_scalar($Src[$Label]) === FALSE )
                return NULL;

            $Src[$Label] = (string) $Src[$Label];
        }
        else
            return !$Required;

        $Src[$Label] = trim(self::SmartStrip($Src[$Label]));

        if( $Src[$Label] === '' )
            return !$Required;

        return ((bool)!preg_match('/[^\011\012\015\040-\176]/',$Src[$Label]))?TRUE:NULL;
    }

    /**
     * Normalize and validate an element's value to be within a set of values.
     *
     * The following is performed:
     *  - Ensure the value exists in the allowable options using strict type checking.
     *  - Options may be provided as an associative or numeric array.  If associative, each element's
     *    key is taken as the value and the value as the label of each option.  If numeric, each
     *    element's value is taken as both the value and label of each option and checked as strings.
     *  - A non-scalar value isn't valid.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param array $Options The array of allowed options.
     * @param boolean $Required Set to FALSE if the element to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @note This can be used for a form's dropdown, radio and single-checkbox fields.
     * @note When options are a numeric array, in_array() type-strict checking is used - ensure the values are strings.
     */
    public static function Single( $Label,&$Src,$Options,$Required = TRUE )
    {
        if( isset($Src[$Label]) )
        {
            if( is_scalar($Src[$Label]) === FALSE )
                return NULL;

            $Src[$Label] = (string) $Src[$Label];
        }
        else
            return !$Required;

        if( Is::Numeric($Options) )
            return in_array($Src[$Label],$Options,TRUE)?TRUE:NULL;
        else if( isset($Options[$Src[$Label]]) )
            return TRUE;
        else
            return NULL;
    }

    /**
     * Normalize and validate an element's array of values to be within a set of values.
     *
     * The following is performed:
     *  - Ensure that all values exist in the allowable options using strict type checking.
     *  - Options may be provided as an associative or numeric array.  If associative, each element's
     *    key is taken as the value and the value as the label of each option.  If numeric, each
     *    element's value is taken as both the value and label of each option and checked as strings.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param array $Options The array of allowed options.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @note This can be used for a form's listboxes and multi-checkboxes fields.
     * @note When options are a numeric array, in_array() type-strict checking is used - ensure the values are strings.
     */
    public static function Multi( $Label,&$Src,$Options,$Required = TRUE )
    {
        if( isset($Src[$Label]) )
        {
            if( is_scalar($Src[$Label]) === FALSE )
                return NULL;

            $Src[$Label] = (string) $Src[$Label];
        }
        else
            return !$Required;

        if( Is::Numeric($Options) )
        {
            foreach( ((array) $Src[$Label]) as $V )
                if( !in_array($Src[$Label],$Options,TRUE) )
                    return NULL;

            return TRUE;
        }
        else
        {
            foreach( ((array) $Src[$Label]) as $V )
                if( !isset($Options[$V]) )
                    return NULL;

            return TRUE;
        }
    }

    /**
     * Retrieve a listing of US states.
     *
     * @note Use with enUS::Single() or enUS::Multi() to validate a submitted form field.
     */
    public static function StateListing()
    {
        return static::$States;
    }

    /**
     * Retrieve a listing of title prefixes.
     *
     * @note Use with enUS::Single() or enUS::Multi() to validate a submitted form field.
     */
    public static function PrefixListing()
    {
        return static::$Prefixes;
    }
    /**
     * Retrieve a listing of title suffixes.
     *
     * @note Use with enUS::Single() or enUS::Multi() to validate a submitted form field.
     */
    public static function SuffixListing()
    {
        return static::$Suffixes;
    }

    /**
     * Normalize and validate an element as a date.
     *
     * This uses PHP's native DateTime functions.
     *
     * IT will be alter the original data to a standard ISO date YYYY-mm-dd.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     */
    public static function Date( $Label,&$Src,$Required = TRUE )
    {
        $R = static::Text($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        try
        {
            $DT = new \DateTime($Src[$Label]);
            $Src[$Label] = $DT->format('Y-m-d');
            return TRUE;
        }
        catch( Exception $E )
        {
            return NULL;
        }
    }

    /**
     * Noramlize and validate an element as a 5 or 9 digit US zip code.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     */
    public static function ZipCode( $Label,&$Src,$Required = TRUE )
    {
        $R = static::Text($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        return preg_match('/^([0-9]{5})(-[0-9]{4})?$/i',$Src[$Label])===0?NULL:TRUE;
    }

    /**
     * Normalize and validate an element as a US phone number.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @note The resulting phone number will be stripped of all delimiters, including a leading plus sign,
     *       thus breaking international notation.
     * @note An extension, indicated by an 'x', is preserved.
     */
    public static function PhoneNumber( $Label,&$Src,$Required = TRUE )
    {
        $R = static::Text($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        $Src[$Label] = str_replace(array(' ','-','+','.',',','|','(',')'),'',$Src[$Label]);
        $ExtPosi = stripos($Src[$Label],'x');

        $Ext = '';
        if( $ExtPosi !== FALSE )
        {
            $Ext = substr($Src[$Label],$ExtPosi);
            $Src[$Label] = substr($Src[$Label],0,$ExtPosi);
        }

        if( ctype_digit($Src[$Label]) === FALSE )
            return NULL;

        if( strlen($Src[$Label]) !== 10 && strlen($Src[$Label]) !== 11 )
            return NULL;

        $Src[$Label] .= $Ext;
        return TRUE;
    }

    /**
     * Normalize and validate an element as a US social security number.
     *
     * The following is performed:
     *  - Dashes, spaces and periods are removed.
     *  - enUS::Number() to normalize and validate the element.
     *  - XXX-XX-XXXX format is checked and slashed added for correct form.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     */
    public static function SSN( $Label,&$Src,$Required = TRUE )
    {
        if( empty($Src[$Label]) )
            return !$Required;

        $Src[$Label] = str_replace(array('-',' ','.'),'',$Src[$Label]);

        $R = static::Number($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        if( preg_match('/^[\d]{3}[\d]{2}[\d]{4}$/',$Src[$Label]) !== 0 )
        {
            $T = $Src[$Label];
            $Src[$Label] = "{$T{0}}{$T{1}}{$T{2}}-{$T{3}}{$T{4}}-{$T{5}}{$T{6}}{$T{7}}{$T{8}}";
            return TRUE;
        }
        else
            return NULL;
    }

    /**
     * Normalize and validate an element as an IP address.
     *
     * The following is performed:
     *  - enUS:Text() to normalize and validate the element.
     *  - filter_var() check using FILTER_VALIDATE_IP.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     */
    public static function IP( $Label,&$Src,$Required = TRUE )
    {
        $R = static::Text($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        $T = filter_var($Src[$Label],FILTER_VALIDATE_IP);
        if( $T === FALSE )
            return NULL;

        $Src[$Label] = $T;

        return TRUE;
    }

    /**
     * Normalize and validate an element as a credit card number.
     *
     * The following is performed:
     *  - Dashes, spaces and periods are removed.
     *  - enUS::Number() to normalize and validate the element.
     *  - Luhn checksum is tested.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @todo Could also use a CCExpiration and CCCVV
     */
    public static function CCNumber( $Label,&$Src,$Required = TRUE )
    {
        if( empty($Src[$Label]) )
            return !$Required;

        $Src[$Label] = str_replace(array('-',' ','.'),'',$Src[$Label]);

        $R = static::Number($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        $Sum = 0;
        $Weight = 2;
        $Len = strlen($Src[$Label]);
        for( $i = $Len - 2; $i >= 0; --$i )
        {
            $Digit = $Weight * $Src[$Label][$i];
            $Sum += floor($Digit/10) + $Digit % 10;
            $Weight = $Weight % 2 + 1;
        }

        if( (10 - $Sum % 10) %10 != $Src[$Label][$Len-1])
            return NULL;
        else
            return TRUE;
    }

    /**
     * Normalize and validate an element as an email addres.
     *
     * The following is performed:
     *  - enUS:Text() to normalize and validate the element.
     *  - filter_var() check using FILTER_VALIDATE_EMAIL.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     */
    public static function Email( $Label,&$Src,$Required = TRUE )
    {
        $R = static::Text($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        $T = filter_var($Src[$Label],FILTER_VALIDATE_EMAIL);
        if( $T === FALSE )
            return NULL;

        $Src[$Label] = $T;

        return TRUE;
    }

    /**
     * Normalize and validate an element as a username.
     *
     * The following is performed:
     *  - enUS:Text() to normalize and validate the element.
     *  - Ensure an alpha-numeric string, including underscores.
     *  - Length must be between 4 and 20 characters.
     *
     * @param string $Label The name of the element to check.
     * @param array $Src The array of data.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     */
    public static function Username( $Label,&$Src,$Required = TRUE )
    {
        $R = static::Text($Label,$Src,$Required);

        if( $R !== TRUE )
            return $R;

        if( empty($Src[$Label]) )
            return !$Required;

        return preg_match('/^[a-z\d_]{4,20}$/i',$Src[$Label])===0?NULL:TRUE;
    }

    /**
     * Normalize and validate an element as a password, optionally confirming it against another field.
     *
     * The following is performed:
     *  - Length must be at least 4 characters.
     *  - enUS::Text() to normalize and validate a single element.
     *  - Or enUS::TextConfirm() to normalize and validate both elements.
     *
     * @param string $Label1 The name of the element to check.
     * @param array $Src The array of data.
     * @param string $Label2 Optional name of password confirmation element.
     * @param boolean $Required Set to FALSE if the element is allowed to be empty (or not exist).
     * @retval boolean TRUE if the element exists and validates or is empty and not required, FALSE if required but empty.
     * @retval NULL Data did not validate.
     *
     * @note Because this uses enUS::Text(), leading and trailing whitespace is stripped, which may lead to
     *       user confusion.
     */
    public static function Password( $Label1,&$Src,$Label2 = NULL,$Required = TRUE )
    {
        if( $Label2 === NULL )
        {
            $R = static::Text($Label1,$Src,$Required);

            if( $R !== TRUE )
                return $R;

            if( empty($Src[$Label1]) )
                return !$Required;
            else
                return (strlen($Src[$Label1]) > 3)===TRUE?TRUE:NULL;
        }
        else
        {
            $R = static::TextConfirm($Label1,$Label2,$Src,$Required);

            if( $R !== TRUE )
                return $R;

            if( empty($Src[$Label1]) )
                return !$Required;
            else
                return (strlen($Src[$Label1]) > 3)===TRUE?TRUE:NULL;
        }
    }
}


trait enUSHTMLTraits
{
    /**
     * Allowed HTML tags when Strip()'ing HTML.
     */
    protected static $AllowTags = '<a><b><i><u><strong><em><span><font><br><strike>
                                   <h2><h3><h4><h5><h6><h7>
                                   <ul><ol><li>
                                   <div><p>
                                   <table><tr><td>
                                   <img><object><param><embed>';

    /**
     * Recognized TLDs for ToLinks() to create links in text.
     */
    protected static $ccTLDs = array('com','org','edu','net','gov','us','tv','ly','io','my','uk','ca','de','dk',
                                     'nl','th','jp','cn','me','info','fm','pr');


    /**
     * HTML encode values.
     *
     * The following functionality is available depending on how it's called:
     *  - $Src is NULL, $Label is the scalar to encode.
     *  - $Src is an array, $Label is element (scalar) in $Src to encode if isset().
     *  - $Check is a scalar, $Label is element in $Src (scalar or array for multi-select
     *    support), compared as strings and if equal $Token is returned not-encoded,
     *    otherwise an empty string is returned.
     *
     * The method is commonly used for encoding strings in templates and pulling/encoding
     * values from an array (such as $_GET or $_POST) in a template.
     *
     * @param string $Label The value to encode or an array's element index.
     * @param array $Src An array to pull the value from.
     * @param NULL $Src $Label is taken as the value.
     * @param scalar $Check Value to check against for $Token.
     * @param NULL $Check No check will be performed.
     * @param string $Token Token to return if $Check is equal to the value.
     * @retval string HTML encoded value, $Token, or an empty string if $Label isn't an element
     *                in $Src, or the value wasn't equal to $Check.
     *
     * @note This uses htmlentities().  $Token is NOT encoded.
     * @note No checking for invalid arguments are performed - an empty string is returned.
     *
     * @todo Add charset/flag awareness with htmlentities().
     * @todo This is called a lot - look at optimizations (is_array).
     */
    public function __invoke( $Label,$Src = NULL,$Check = NULL,$Token = NULL )
    {
        // $Label is value to encode, no check
        if( $Src === NULL && $Check === NULL )
        {
            return htmlentities($Label);
        }
        // $Label is value to encode, check
        else if( $Src === NULL && is_scalar($Check) )
        {
            return $Label===((string)$Check)?$Token:'';
        }
        // $Label is element (scalar) to encode, no check
        else if( is_array($Src) && $Check === NULL )
        {
            return isset($Src[$Label])?htmlentities($Src[$Label]):'';
        }
        // $Label is element (scalar/array) to encode, check
        else if( is_array($Src) && is_scalar($Check) )
        {
            if( isset($Src[$Label]) )
            {
                // handle multi-selects/checkboxes
                if( is_array($Src[$Label]) )
                    return in_array($Check,$Src[$Label],TRUE)?$Token:'';
                else
                    return $Src[$Label]===$Check?$Token:'';
            }
            else
                return '';
        }
        // we give up
        else
            return '';
    }

    /**
     * Strips HTML tags from a string, allowing certain tags to remain.
     *
     * @param string $Src String to remove HTML tags from.
     * @param string $AllowTags Optional set of allowed tags, in the form '\<b\>\<i\>\<u\>...'.
     * @retval string The string with HTML tags removed.
     */
    public static function Strip( $Src,$AllowTags = NULL )
    {
        if( is_string($AllowTags) )
            $AllowTags = str_replace(array(" ","\r","\n"),'',$AllowTags);
        else
            $AllowTags = str_replace(array(" ","\r","\n"),'',static::$AllowTags);

        return strip_tags($Src,$AllowTags);
    }

    /**
     * Add HTML links (<a href...>) to a string.
     *
     * This method uses a predefined set of top-level domain names to identify where links should be created.
     *
     * @param string $Src String to add links to.
     * @param array $ccTLDs Optional set of ccTLDs, in the form array('com','edu','gov'...)
     * @retval string The string with HTML links added.
     */
    public static function ToLinks( $Src,$ccTLDs = NULL )
    {
        if( is_array($ccTLDs) )
            $ccTLDs = implode('|',$ccTLDs);
        else
            $ccTLDs = implode('|',static::$ccTLDs);

        return preg_replace("#(^|[\n ])([\w]+?://)?([^@]([A-Z0-9-.])+\.($ccTLDs)[^ \"\t\n\r<]*)#ise", "'\\1<a href=\"http://\\3\" >\\3</a>'",$Src);
    }
}

/**
 * A set of English (latin) HTML templates and tools.
 *
 * This is not character set encoding aware and uses standard PHP
 * string functions.
 *
 * @see TemplateSet
 */
class enUSHTMLSet extends TemplateSet
{
    use enUSHTMLTraits;
}


