#ifndef MUTGOS_TEXT_EXTERNALURLTEXT_H
#define MUTGOS_TEXT_EXTERNALURLTEXT_H

#include <string>

#include "text/text_ExternalText.h"

namespace mutgos
{
namespace text
{
    /**
     * Represents a URL, providing hints as to how best to handle the URL
     * when clicked.
     */
    class ExternalUrlText : public ExternalText
    {
    public:
        /**
         * Indicates type of URL this is, to provide a hint to clients as
         * to how to display it.
         * Update ExternalTextConverter if this enum changes.
         */
        enum UrlType
        {
            /** For URLs that go to web pages.  Must always be first enum entry. */
            URL_TYPE_PAGE,
            /** For URLs that go to an image */
            URL_TYPE_IMAGE,
            /** For URLs that go to sound/music */
            URL_TYPE_AUDIO,
            /** Internal use only.  Insert new enums above this. */
            URL_TYPE_END_INVALID
        };

        /**
         * Creates an external URL text, suitable for deserialization.
         */
        ExternalUrlText(void)
            : ExternalText(ExternalText::TEXT_TYPE_URL),
              url_type(URL_TYPE_END_INVALID)
        {  }

        /**
         * Creates an external URL text.
         * @param url_type[in] The type of URL, to provide a hint to the client
         * as to how it should be displayed.
         * @param url[in] The URL.
         * @param name[in] A name/title/description for the URL.
         */
        ExternalUrlText(
            const UrlType url_type,
            const std::string url,
            const std::string name)
            : ExternalText(ExternalText::TEXT_TYPE_URL),
              url_type(url_type),
              url_text(url),
              url_name(name)
          {  }

        /**
         * Copy constructor.
         * @param rhs[in] The source to copy from.
         */
        ExternalUrlText(const ExternalUrlText &rhs)
            : ExternalText(rhs),
              url_type(rhs.url_type),
              url_text(rhs.url_text),
              url_name(rhs.url_name)
          { }

        /**
         * Destructor.
         */
        virtual ~ExternalUrlText()
          { }

        /**
         * Creates a copy of this ExternalText.
         * @return A cloned copy.  Caller must manage the pointer.
         */
        virtual ExternalText *clone(void) const
        {
            return new ExternalUrlText(*this);
        }

        /**
         * Saves this to the provided JSON node.
         * @param root[in] The JSON root document.
         * @param node[out] The JSON node in which to save state.
         * @return True if success.
         */
        virtual bool save(json::JSONRoot &root, json::JSONNode &node) const;

        /**
         * Restores this from the provided JSON node.
         * @param node[in] The JSON node to restore state from.
         * @return True if success.
         */
        virtual bool restore(const json::JSONNode &node);

        /**
         * @return The type of URL.
         */
        const UrlType get_url_type(void) const
          { return url_type; }

        /**
         * @return The URL.
         */
        const std::string &get_url(void) const
          { return url_text; }

        /**
         * @return The name/title of the URL.
         */
        const std::string &get_url_name(void) const
          { return url_name; }

        /**
         * @return The ExternalText component as a plain string, stripping
         * all metadata or other formatting.
         */
        virtual std::string to_string(void) const
          { return get_url() + " (" + get_url_name() + ")"; }

    private:
        UrlType url_type; ///< Type of URL
        std::string url_text; ///< The URL itself
        std::string url_name; ///< Name/description of the URL
    };
}
}


#endif //MUTGOS_TEXT_EXTERNALURLTEXT_H
