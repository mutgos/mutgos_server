// This component will render external formatted text into
// HTML / CSS to display the equivalent color and style.
//
// Simply call mutgos_registerFormattedTextComponent() to register
// the component with Vue.
//

mutgos_registerFormattedTextComponent = function() {
/**
 * Converts an integer color into a two digit hex code suitable
 * for a CSS color.
 * @param {number} colorInt The color as an integer (0-255).
 * @return {string} The color as a two digit hex code.
 */
var toHexColor = function(colorInt) {
    var color;

    color = colorInt.toString(16);

    if (color.length === 1) {
        color = "0" + color;
    }

    return color;
};

/**
 * Converts the color fields of a ExternalFormattedText into a string for
 * CSS color.
 * @param {ExternalFormattedText} externalFormattedText The
 * ExternalFormattedText to render.
 * @return {string} The color string to use.
 */
var convertColor = function(externalFormattedText) {
    var color;

    switch (externalFormattedText.color) {
        case externalFormattedText.ColorEnum.CUSTOM:
            // Convert RGB values to web
            //
            color = "#";
            color += toHexColor(externalFormattedText.red);
            color += toHexColor(externalFormattedText.green);
            color += toHexColor(externalFormattedText.blue);
            break;

        case externalFormattedText.ColorEnum.DEFAULT:
        case externalFormattedText.ColorEnum.UNKNOWN:
            // Default color
            color = "";
            break;

        default:
            // All other choices directly correspond to a valid HTML color
            color = externalFormattedText.color;
            break;
    }

    return color;
};

/**
 * Returns a Vue element suitable for rendering plain text.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {ExternalPlainText} externalPlainText The ExternalPlainText to render.
 * @return {Object} The Vue element.
 */
var renderPlainText = function(createElement, externalPlainText) {
    return createElement(
        'div',
        externalPlainText.text);
};

/**
 * Returns a Vue element suitable for rendering plain text.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {ExternalFormattedText} externalFormattedText The
 * ExternalFormattedText to render.
 * @return {Object} The Vue element.
 */
var renderFormattedText = function(createElement, externalFormattedText) {
    var textElement = createElement('div', externalFormattedText.text);

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
            'div',
            {
                style: {
                    color: "black",
                    backgroundColor : color
                }
            },
            [ textElement ]);
    } else {
        textElement = createElement(
            'div',
            {
                style: {
                    color: color
                }
            },
            [ textElement ]);
    }

    return textElement;
};

var renderUrlText = function(createElement, externalText) {
// TODO
};

var renderIdText = function(createElement, externalText) {
// TODO
};

/**
 * Given an ExternalText instance, return a Vue element suitable for rendering.
 * This is basically a helper class that calls the right translator method
 * based on the text type.
 * @param {function} createElement A Vue-provided function to create an element.
 * @param {ExternalText} externalText An ExternalText or subclass to render.
 * @return {Object} The Vue element.
 */
var renderExternalTextElement = function(createElement, externalText) {
    var result;

    switch (externalText.textType) {
        case externalText.TextTypeEnum.PLAIN:
            result = renderPlainText(createElement, externalText);
            break;

        case externalText.TextTypeEnum.FORMATTED:
            result = renderFormattedText(createElement, externalText);
            break;

        case externalText.TextTypeEnum.ID:
            result = renderIdText(createElement, externalText);
            break;

        case externalText.TextTypeEnum.URL:
            result = renderUrlText(createElement, externalText);
            break;

        default:
            result = createElement(
                "h4",
                "Unknown text type: " + externalText.textType);
            break;
    }

    return result;
};


Vue.component(
    'formatted-text-line',
    {
        props: ['externalText'],
        render: function(createElement) {
            // TODO iterate through text and call renderExternalTextElement()

            // return createElement('h1', this.rawText);
        }
    });

};
