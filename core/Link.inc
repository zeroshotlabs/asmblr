<?php
/**
 * @file Link.inc URL creation.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * Create well-formed URLs.
 *
 * A LinkSet creates URLs calculated from a base URL.  Created URLs may also contain
 * one-time changes, merged on the fly at the time of creation.
 *
 * A LinkSet is instantiated with a base URL and optional array of changes:
 *     $ls = new LinkSet('www.stackware.com',array('district'=>'nyc'));
 *     $ls = new LinkSet('www.stackware.com','district=nyc');
 *
 * Both lines do the same thing - all created URLs will be, by default, based on:
 *     http://www.stackware.com/?district=nyc
 *
 * The $ls object is then called as a function with additional changes for create URLs:
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
class LinkSet
{
    /**
     * The URL Struct that links will be derived from.
     */
    protected $BaseURL = NULL;

    /**
     * BaseURL cached as a string.
     *
     * @note Not currently implemented because of Set() complexity.
     */
    protected $BaseURLStr = '';


    /**
     * LinkSet constructor.
     *
     * If no BaseURL is supplied, it will default to the current SiteURL (App::$SiteURL).
     * The BaseURL and any changes set here, will persist for all created URLs, unless
     * changed using SetBaseURL() or Set().
     *
     * @param array $BaseURL URL Struct to use as BaseURL.
     * @param string $BaseURL URL string to use as BaseURL.
     * @param NULL $BaseURL App::$SiteURL will be used.
     * @param array $Set Array of change strings for persistent changes to BaseURL.
     */
    public function __construct( $BaseURL = NULL,$Set = array() )
    {
        $this->SetBaseURL($BaseURL,$Set);
    }

    /**
     * Return the base URL as a string.
     *
     * @retval string The current BaseURL.
     */
    public function __toString()
    {
        return URL::ToString($this->BaseURL);
    }

    /**
     * Build a URL for the provided Path or filename.
     *
     * The URLs is calculated from BaseURL and may incorporate one-time changes.
     *
     * @param string $File A filename with optional path or an empty string to use only the base URL.
     * @param array $Set Array of change strings for one-time changes to BaseURL.
     * @retval string A well-formed URL.
     */
    public function __invoke( $File = '',$Set = array() )
    {
        $Base = $this->BaseURL;

        if( !empty($File) )
            Path::Merge(Path::Init($File),$Base['Path']);

        if( !empty($Set) )
            URL::Set($Set,$Base);

        return URL::ToString($Base);
    }

    /**
     * Set the BaseURL and optionally apply changes to it.
     *
     * The BaseURL and changes supplied here will persist for all
     * URLs formed by the object.
     *
     * @param URL $BaseURL URL Struct to use as BaseURL.
     * @param string $BaseURL URL string to use as BaseURL.
     * @param array $Set Array of change strings for persistent changes to BaseURL.
     * @throws Exception Invalid base URL.
     *
     * @note Caches BaseURL as $BaseURLStr.
     */
    public function SetBaseURL( $BaseURL,$Set = array() )
    {
        if( is_string($BaseURL) )
            $this->BaseURL = URL::Init($BaseURL);
        else if( is_array($BaseURL) )
            $this->BaseURL = $BaseURL;
        else
            throw new Exception('Invalid base URL.');

        if( !empty($Set) )
            URL::Set($Set,$this->BaseURL);

        $this->BaseURLStr = URL::ToString($this->BaseURL);
    }

    /**
     * Make persistant changes to the BaseURL.
     *
     * @param array $Set Array of change strings for persistent changes to BaseURL.
     *
     * @see URL::Set() for details on valid change string syntax.
     */
    public function Set( $Set )
    {
       URL::Set($Set,$this->BaseURL);
    }

    /**
     * Get the object's current BaseURL Struct.
     *
     * @retval array The BaseURL Struct.
     */
    public function GetBaseURL()
    {
        return $this->BaseURL;
    }

    /**
     * Alias of __invoke() for creating a URL.
     *
     * @see LinkSet::__invoke().
     */
    public function Link( $File = NULL,$Set = array() )
    {
        return $this->__invoke($File,$Set);
    }

    /**
     * Form URL and perform a permanent redirect to it.
     *
     * @param string $File A filename with optional path or an empty string to use only the base URL.
     * @param array $Set Optional on-the-fly URL changes to apply.
     */
    public function Go( $File = NULL,$Set = array() )
    {
        HTTP::Location($this->__invoke($File,$Set));
    }
}


/**
 * Create well-formed URLs for Pages.
 *
 * A LinkPage creates URLs for the Pages of a PageSet, which is calculated from
 * a BaseURL (usually App::$SiteURL).  Created URLs may also contain
 * one-time changes, merged on the fly at the time of creation.
 *
 * A LinkPage is instantiated with a PageSet, base URL and optional array of
 * changes:
 *     $lp = new LinkPage($ps,$this->SiteURL,'district=nyc');
 *
 * If SiteURL has a Path of /prefix-path/, all created URLs will be,
 * by default, based on:
 *     http://www.stackware.com/prefix-path/?district=nyc
 *
 * The $lp object is then called as a function to create URLs, with the first
 * argument being the Name of a Page and the optional second argument being an
 * array of one-time changes:
 *     $lp('Register',array('>newyork','\#form','src=home'));
 *
 * If the Register Page had a Path of /register, this would result
 * in the URL:
 *     http://www.stackware.com/prefix-path/register/newyork?district=nyc&src=home#form
 *
 * @see URL::Set() for details on the change string syntax.
 * @note All path segments and query string keys/values are properly encoded.
 *       Hostname/port/scheme are not encoded.
 */
class LinkPage extends LinkSet
{
    protected $App;
    protected $PageSet;


    /**
     * LinkPage constructor.
     *
     * If no BaseURL is supplied, it will default to the current SiteURL (App::$SiteURL).
     * The BaseURL and any changes set here, will persist for all created URLs, unless
     * changed using SetBaseURL() or Set().
     *
     * @param PageSet $PageSet PageSet containing Pages to create URLs for.
     * @param array $BaseURL URL Struct to use as BaseURL.
     * @param string $BaseURL URL string to use as BaseURL.
     * @param NULL $BaseURL App::$SiteURL will be used.
     * @param array $Set Array of change strings for persistent changes to BaseURL.
     */
    public function __construct( \asm\PageSet $PageSet,\asm\App $App,$BaseURL = NULL,$Set = array() )
    {
        $this->PageSet = $PageSet;
        $this->App = $App;

        $this->SetBaseURL($BaseURL,$Set);
    }

    /**
     * Build a URL for the provided Page.
     *
     * The URL is calculated from BaseURL and may incorporate one-time changes.
     *
     * @param string $Name The Name of a Page.
     * @param NULL $Name Use the request's currently executing Page.
     * @param array $Set Array of change strings for persistent changes to BaseURL.
     * @retval string A well-formed URL.
     *
     * @todo Most links created are of the current request with a change in query string
     *       and/or fragment.  We could cache the $BaseURL and last executed Page and
     *       add only query string/fragment.  Another strategy would be to "freeze"
     *       certain parts of the URL, based on the type of URLs we're forming - for example,
     *       if the path changes, rather than the query string, when forming long lists of links.
     *       This behavior would need to include which URLs parts the Set arg includes -
     *       the more Set, the less caching.  Another to consider is that once this
     *       starts executing (and Current()) the current Page likely won't change, and thus
     *       can be cached.  Note that these caches probably don't belong in LinkSet itself.
     */
    public function __invoke( $Name = NULL,$Set = array() )
    {
        static $CurrentURLStr = '';

        // Most URLs formed are based on the current request so cache that
        // though this just left here for future reference and not implemented yet.
//         if( $CurrentURLStr === '' )
//         {
//             $P = end($this->PageSet->Executed);
//             // we have a 404...?
// //            if( $P === FALSE )

//             $Base = $this->BaseURL;
//             Path::Merge($P['PathStruct'],$Base['Path']);
//             $this->CurrentURLStr = URL::ToString($Base);
//         }

        $Base = $this->BaseURL;

        // try to detect the last executed Page or default to the SiteURL's path
        // No executed Pages, i.e. a 404 - use the SiteURL's path

        if( empty($Name) )
        {
            // should rarely happen
            if( empty($this->App->ClosestMatchName) )
                $P = array('PathStruct'=>$this->App->Request['SiteURL']['Path'],'Name'=>'Home');
            else
                $Name = $this->App->ClosestMatchName;
        }
        // page isn't known
        else if( empty($this->PageSet->Pages[$Name]) )
        {
            Path::Append("PAGE-{$Name}-NOT-FOUND",$Base['Path']);
            return URL::ToString($Base);
        }

        Path::Merge($this->PageSet->Pages[$Name]['PathStruct'],$Base['Path']);

        // apply Set() change strings
        if( !empty($Set) )
        {
            // this currently will break other change strings at the same time
            // see URL::Set() - this all could use some optimization
            if( strpos($Set,'/') )
            {
                $Set = explode('/',$Set);
                foreach( $Set as &$V )
                    $V = ">{$V}";
            }

            URL::Set($Set,$Base);
        }

        return URL::ToString($Base);
    }

    /**
     * Test whether one or more Pages have been matched for this request.
     *
     * $True or $False is returned accordingly.
     *
     * @param string $Name Page Name to test.
     * @param array $Name Page Names to test.
     * @param mixed $True Value to return if Name is executing.
     * @param mixed $False Value to return if Name is not executing.
     * @retval mixed Value of $True or $False parameter.
     *
     * @note Matching multiple pages is slower if overused - use a hierarchal page like /admin/ instead.
     * @note When checking multiple pages, if any matches, $True is returned.
     * @todo This checks the most exact matched page.  We may add the option to specify an exact or ordered match.
     */
    public function Current( $Name,$True = 'active',$False = NULL )
    {
        if( is_string($Name) )
        {
            return $this->App->ClosestMatchName===$Name?$True:$False;
        }
        else
        {
            foreach( $Name as $N )
            {
                if( $this->App->ClosestMatchName === $Name )
                    return $True;
            }

            return $False;
        }
    }
}

