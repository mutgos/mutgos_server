// This component will render external formatted text into
// HTML / CSS to display the equivalent color and style.
//
// Simply call mutgos_registerFormattedTextComponent() to register
// the component with Vue.
//

mutgos_registerFormattedTextComponent = function() {
/**
 * @private
 * Converts an integer color into a two digit hex code suitable
 * for a CSS color.
 * @param {number} colorInt The color as an integer (0-255).
 * @return {string} The color as a two digit hex code.
 */
var toHexColor = function(colorInt) {
    var color;

    color = colorInt.toString(16);

    if (color.length === 1) {
        color = '0' + color;
    }

    return color;
};

/**
 * @private
 * Converts the color fields of a ExternalFormattedText into a string for
 * CSS color.
 * @param {ExternalFormattedText} externalFormattedText The
 * ExternalFormattedText to render.
 * @return {string} The color string to use.
 */
var convertColor = function(externalFormattedText) {
    var color;

    switch (externalFormattedText.color) {
        case ExternalFormattedText.prototype.ColorEnum.CUSTOM:
            // Convert RGB values to web
            //
            color = '#';
            color += toHexColor(externalFormattedText.red);
            color += toHexColor(externalFormattedText.green);
            color += toHexColor(externalFormattedText.blue);
            break;

        case ExternalFormattedText.prototype.ColorEnum.DEFAULT:
        case ExternalFormattedText.prototype.ColorEnum.UNKNOWN:
            // Default color
            color = '';
            break;

        default:
            // All other choices directly correspond to a valid HTML color
            color = externalFormattedText.color;
            break;
    }

    return color;
};

/**
 * @private
 * Returns a Vue element suitable for rendering plain text.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {ExternalPlainText} externalPlainText The ExternalPlainText to render.
 * @return {Object} The Vue element.
 */
var renderPlainText = function(createElement, externalPlainText) {
    return createElement(
        'span',
        externalPlainText.plainText);
};

/**
 * @private
 * Returns a Vue element suitable for rendering plain text.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {ExternalFormattedText} externalFormattedText The
 * ExternalFormattedText to render.
 * @return {Object} The Vue element.
 */
var renderFormattedText = function(createElement, externalFormattedText) {
    var textElement = createElement('span', externalFormattedText.plainText);

    if (externalFormattedText.bold) {
        textElement = createElement('strong', [textElement]);
    }

    if (externalFormattedText.italic) {
        textElement = createElement('em', [textElement]);
    }

    if (externalFormattedText.underline) {
        textElement = createElement('u', [textElement]);
    }

    var color = convertColor(externalFormattedText);

    if (externalFormattedText.inverse) {
        // Inspired from https://stackoverflow.com/a/42586457
        textElement = createElement(
            'span',
            {
                style: {
                    color: 'black',
                    backgroundColor : color
                }
            },
            [ textElement ]);
    } else {
        textElement = createElement(
            'span',
            {
                style: {
                    color: color
                }
            },
            [ textElement ]);
    }

    return textElement;
};

/**
 * @private
 * Returns a Vue element suitable for rendering URL text.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {ExternalUrlText} externalUrlText The ExternalUrlText to render.
 * @return {Object} The Vue element.
 */
var renderUrlText = function(createElement, externalUrlText) {
    // For now, all URLs will be treated the same.
    return createElement(
        'a',
        {
            attrs: {
                href: externalUrlText.url
            }
        },
        externalUrlText.name
    );
};

/**
 * @private
 * Returns a Vue element suitable for rendering ID text.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {Object} component The component reference.
 * @param {ExternalIdText} externalIdText The ExternalIdText to render.
 * @return {Object} The Vue element.
 */
var renderIdText = function(createElement, component, externalIdText) {
    var idElement;

    switch (externalIdText.type) {
        case ExternalIdText.prototype.IdTypeEnum.ACTION:
        case ExternalIdText.prototype.IdTypeEnum.EXIT:
            idElement = createElement(
                'a',
                {
                    on: {
                        // From https://stackoverflow.com/a/36968187
                        click: component.useExit
                    },
                    attrs: {
                        // Not the best way to handle this.  Anyone know
                        // of something cleaner?
                        href: 'javascript:void(0);',

                        entitySite: externalIdText.id.siteId,
                        entityId: externalIdText.id.entityId
                    }
                },
                externalIdText.name
            );
            break;

        case ExternalIdText.prototype.IdTypeEnum.ENTITY:
            idElement = createElement(
                'a',
                {
                    on: {
                        // From https://stackoverflow.com/a/36968187
                        click: component.submitLookRequest
                    },
                    attrs: {
                        // Not the best way to handle this.  Anyone know
                        // of something cleaner?
                        href: 'javascript:void(0);',

                        entitySite: externalIdText.id.siteId,
                        entityId: externalIdText.id.entityId
                    }
                },
                externalIdText.name
            );
            break;

        default:
            // Unknown type
            idElement = createElement(
                'span',
                externalIdText.name);
            break;
    }

    return idElement;
};

/**
 * @private
 * Given an ExternalText instance, return a Vue element suitable for rendering.
 * This is basically a helper class that calls the right translator method
 * based on the text type.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {Object} component The component reference.
 * @param {ExternalText} externalText An ExternalText or subclass to render.
 * @return {Object} The Vue element.
 */
var renderExternalTextElement = function(createElement, component, externalText) {
    var result;

    switch (externalText.textType) {
        case ExternalText.prototype.TextTypeEnum.PLAIN:
            result = renderPlainText(createElement, externalText);
            break;

        case ExternalText.prototype.TextTypeEnum.FORMATTED:
            result = renderFormattedText(createElement, externalText);
            break;

        case ExternalText.prototype.TextTypeEnum.ID:
            result = renderIdText(createElement, component, externalText);
            break;

        case ExternalText.prototype.TextTypeEnum.URL:
            result = renderUrlText(createElement, externalText);
            break;

        default:
            result = createElement(
                'h4',
                'Unknown text type: ' + externalText.textType);
            break;
    }

    return result;
};


Vue.component(
    'formatted-text-line',
    {
        props: ['external-text-topic', 'external-text-id'],
        render: function(createElement) {
            // Iterate through JSON and call renderExternalTextElement() on
            // each element
            //
            var renderedElements = [];
            var externalText =  outputTextStore[this.externalTextTopic]
                [Number(this.externalTextId)].textLine;

            for (var index = 0; index < externalText.length; ++index) {
                renderedElements.push(renderExternalTextElement(
                    createElement,
                    this,
                    externalText[index]));
            }

            // renderedElements.push(createElement('br'));

            // Now we have an array of rendered parts of the line.  Wrap it
            // together and return.

            return createElement('div', renderedElements);
        },
        methods: {
            submitLookRequest : function(event) {
                // Simply call parent component version, since it will know
                // what to do.
                this.$parent.submitLookRequest(event);
            },
            useExit : function(event) {
                // Simply call parent component version, since it will know
                // what to do.
                this.$parent.useExit(event);
            }
        }
    });
};
