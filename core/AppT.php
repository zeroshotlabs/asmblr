<?php
/**
 * @file AppT.php asmblr application controller with theming.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * App controller with theming - will likely fully replace standard App
 */
#[\AllowDynamicProperties]
abstract class AppT extends \asm\App implements Debuggable
{
    use Debugged;

    /**
     * @var array $Theme
     * An active theme, if set.
     *
     * @note Read-only.
     */
    public $Theme;


    /**
     * Default system start-up.
     *
     * Builds default app specific objects and config parameters.  This can be modified
     * as needed.
     *
     * @note parent::__construct() must be called first.
     */
    public function __construct( $App,$Request )
    {
        // @todo tmp hack - probably need path config or anything except this
        $t = APP_ROOT."/templates";

        $this->html = new \asm\enUSHTMLSet($this,$t);

        parent::__construct($App,$Request);


        // general purpose key/value store for in-template info (meta-tags, CSS classes, etc)
        // @note changed - now the page is always created in the template set and we just link to it
        $this->page = $this->html->page;

        // session based UI messages/alerts
        $this->msg = new \asm\Messager;

        // track form/request validation using a default 'has-error' CSS class
        $this->vr = new \asm\ValidationReport('has-error');

        // templates need access to some of these Connect() them in
        // if you later Connect() something using the same name it will overwrite the object here
        // @todo maybe decouple this a bit, move msg/etc to the templateset constructor
        $this->html->Connect(array('lp'=>$this->lp,'lc'=>$this->lc,'msg'=>$this->msg,'vr'=>$this->vr));
    }


    /**
     * Match pages, apply app directives, execute SitewideFunction, execute pages, and render templates.
     *
     * This enforces the @c ForceBaseHostname and @c ForceHTTPS config settings.
     *
     * @throws Exception Directive object doesn't exist.
     * @throws Exception Use ExecuteCLI() for appropriate execution.
     *
     * @note Theme matching is done based on the OS filesystem and original case of the request (like cnvyr, unlike pages).
     * @todo Make theme matching case-insensitive.  maybe.
     */
    public function Execute()
    {
        if( $this->Request['IsCLI'] === TRUE )
            throw new Exception('Use ExecuteCLI().');

        if( (!empty($this->Config['ForceBaseHostname']) && $this->Request['IsBaseHostname'] === FALSE) )
        {
            $this->Request['Hostname'] = $this->Request['BaseURL']['Hostname'];
            HTTP::Location($this->Request);
        }

        if( (!empty($this->Config['ForceHTTPS']) && $this->Request['Scheme'] !== 'https') )
        {
            $this->Request['Scheme'] = 'https';
            HTTP::Location($this->Request);
        }

        // match pages against the MatchPath to determine our executing page(s)
        $this->OrderedMatch = $this->ExactMatch = array();
        $this->ClosestMatchName = '';

        // we'll use a copy of MatchPath and have it lowercased and have another copy that's the original case for theming
        $MatchPath = $MatchPathCase = $this->Request['MatchPath'];
        \asm\Path::Lower($MatchPath);

        // first determine hierarchal ordered matches - most general to most specific URL path
        // only one page will match
        if( $MatchPath['IsRoot'] === FALSE )
        {
            foreach( \asm\Path::Order($MatchPath) as $V )
            {
                if( ($this->OrderedMatch = $this->ps->Match($V)) !== NULL )
                {
                    $this->ClosestMatchName = $this->OrderedMatch['Name'];
                    break;
                }
            }
        }

        // if an ordered match isn't found, determine an exact match.  only one page would match
        // if the ordered match is Weak, an exact match is also allowed
        // i.e.  default:  /admin/ matches then a page with /admin/something will NOT match
        //          Weak:  /admin/ matches then a page with /admin/something WILL match
        if( empty($this->OrderedMatch) || $this->OrderedMatch['Status'] === 'Weak' )
        {
            if( ($this->ExactMatch = $this->ps->Match(\asm\Path::ToString($MatchPath))) !== NULL )
                $this->ClosestMatchName = $this->ExactMatch['Name'];
        }

        // apply app-wide directives
        foreach( $this->Manifest['Directives'] as $V )
        {
            if( empty($this->{$V[0]}) )
                throw new Exception("Directive object '{$V[0]}' doesn't exist while executing app for '{$this->Config['Hostname']}'.");
            else
                $this->{$V[0]}->ApplyDirective($V[1],$V[2]);
        }

        // execute SitewideFunction - returning FALSE will bypass default page checking, execution and HTML rendering, below
        if( !empty($this->Config['SitewideFunction']) )
            $PreContinue = $this->Config['SitewideFunction']($this);
        else
            $PreContinue = TRUE;

        // returning FALSE from SitewideFunction above will bypass default page checking, execution and HTML rendering
        if( $PreContinue !== FALSE )
        {
            // if an exact match, no theming will happen and manifest pages are executed normally
            if( !empty($this->ExactMatch) )
            {
                // not active, 404 regardless of ordered match
                if( $this->ExactMatch['Status'] !== 'Active' )
                    $this->NoPageHandler();

                // execute an ordered match if exists
                if( !empty($this->OrderedMatch) )
                    $this->ps->Execute($this->OrderedMatch);

                // and the exact match
                $this->ps->Execute($this->ExactMatch);

                // and finally begin rendering at the Base.tpl template
                $this->html->Base();
            }
            // otherwise we may have an ordered match and/or theming
            else
            {
                // var_dump($this->OrderedMatch);
                // var_dump($this->ExactMatch);

                // we have an ordered match (and no exact match)
                if( !empty($this->OrderedMatch) )
                {
                    // if not weak just execute and render and nothing else - this handles cnvyr for example
                    if( $this->OrderedMatch['Status'] === 'Active' )
                    {
                        $this->ps->Execute($this->OrderedMatch);
                        $this->html->Base();
                    }
                    // a weak ordered match so execute and determine if there's a theme request based on the adjusted request path
                    else if( $this->OrderedMatch['Status'] === 'Weak' )
                    {
                        // pull out the URL of the ordered match - we use the original case version here and below
                        \asm\Path::Mask($this->OrderedMatch['PathStruct'],$MatchPathCase);

                        // execute first since it might set the theme, etc.
                        // if it returns TRUE "root" requests will be normally rendered, rather use the Theme's Index
                        // in such cases the index should be renamed because it'll be available, for example as /index.html
                        // TODO: return value is important here - this needs work since not sure why it works now
                        if( ($this->ps->Execute($this->OrderedMatch) !== TRUE && $MatchPathCase['IsRoot'] === TRUE) || empty($this->Theme) )
                        {
                            $this->html->Base();
                        }
                        // we're expecting a theme request so give it a try
                        // TODO: we should just let the ordered match handle this?!?!?!
                        // though maybe not for mid-size installs - note that for compact installs
                        // nested themeing wouldn't possible as currently things are
                        // NOTE: we must use the match path with the original case
                        else
                        {
                            $this->TryTheme(\asm\Path::ToString($MatchPathCase));
                        }
                    }
                    // some other status so it's a 404
                    else
                    {
                        $this->NoPageHandler();
                    }
                }
                // no exact and no ordered so attempt a theme otherwise it'll 404
                else
                {
                    $this->TryTheme(\asm\Path::ToString($MatchPathCase));
                }
            }
        }
    }

    /**
     * Executes a single named function.
     * 
     * @note No hierarchal path matching is done, nor is the SitewideFunction executed.  Sitewide directives are run.
     */
    public function ExecuteCLI( string $Page )
    {
        // apply app-wide directives
        foreach( $this->Manifest['Directives'] as $V )
        {
            if( empty($this->{$V[0]}) )
                throw new Exception("Directive object '{$V[0]}' doesn't exist while executing app for '{$this->Config['Hostname']}'.");
            else
                $this->{$V[0]}->ApplyDirective($V[1],$V[2]);
        }

        $this->ps->Execute($Page);
    }

    /**
     * Determine whether the application is running from the command line.
     * 
     * @retval bool TRUE is the application is running from the command line.
     * 
     * @note Based on argv/argc.
     */
    public function IsCLI()
    {
        return $this->Request['IsCLI'];
    }

    // will 404 if the file isn't found
    public function TryTheme( $MatchPathStr )
    {
        $ContentType = HTTP::Filename2ContentType($MatchPathStr);

        // we don't know what it is
        if( $ContentType === 'application/octet-stream' )
            HTTP::_400();

        $FSPath = $this->Theme['DOC_ROOT'].$MatchPathStr;

        if( $this->IsDebug() )
            llog("Theme FS Path: $FSPath\nMatch Path: $MatchPathStr\nContent Type: $ContentType");

        // if we don't find the file then it's a 404
        if( !is_file($FSPath) )
        {
            $this->NoPageHandler();
        }
        // we found it so passthru and no rendering will happen
        // @todo implement nginx passthru xcnvyr style
        else
        {
            HTTP::ContentType($ContentType);
            readfile($FSPath);
        }
    }

    /**
     * Set the active theme.
     *
     * Only one theme can be active at a time, though they can be changed on the fly.
     *
     * @param string $Name Case-sensitive theme (directory) name.
     * @param NULL $Name Deactivate the theme system.
     * @param string $Index Optional case-sensitive theme index file for root requests.
     * @throws Exception Requested theme doesn't exist.
     *
     * @note The theme index file doesn't actually need to exist but should always be set.
     *
     * @todo Root is hardwired approot/themes/
     * @todo Perhaps remove check of doc root since this could be used on strange filesystems.
     */
    public function SetTheme( $Name,$Index )
    {
        if( empty($Name) )
        {
            $this->Theme = array();
            return;
        }

        $Root = $this->Manifest['AppRoot'].'themes/';
        $T = array('Root'=>$Root,'DOC_ROOT'=>$Root.trim($Name).'/','Name'=>trim($Name),'Index'=>trim($Index));

        if( !is_dir($T['DOC_ROOT']) )
            throw new Exception("Requested theme '{$T['Name']}' doesn't exist.");

        $this->Theme = $T;
    }
}
