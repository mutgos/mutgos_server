/*
 * events_ChannelControlListener.h
 */

#ifndef MUTGOS_EVENTS_CHANNELCONTROLLISTENER_H
#define MUTGOS_EVENTS_CHANNELCONTROLLISTENER_H

#include <string>

namespace mutgos
{
namespace events
{
    // Forward declarations.
    //
    class Channel;

    /**
     * Classes that want to get a direct callback (instead of a message in the
     * executor) when the flow or control status has changed must implement
     * this class.  Because the timing of this callback depends on the caller
     * on the other end of the channel, the implementor must be able to
     * accept callbacks at any time.
     */
    class ChannelControlListener
    {
    public:
        /**
         * Interface class constructor.
         */
        ChannelControlListener(void)
         { }

        /**
         * Interface class required destructor.
         */
        virtual ~ChannelControlListener()
         { }

        /**
         * Called when the channel's flow has been blocked, prohibiting items
         * from being placed on it.  Items can be placed on the channel
         * once it has been unblocked.
         * While it is not required, it is highly recommended (to avoid
         * potential stack overflow) to not send a message on the channel
         * when in this callback, if the channel will have flow turned on
         * and off repeatedly.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_flow_blocked(
            const std::string &channel_name,
            Channel * const channel_ptr) =0;

        /**
         * Called when the channel's flow has been opened (unblocked), allowing
         * for one or more items to be placed on it.
         * While it is not required, it is highly recommended (to avoid
         * potential stack overflow) to not send a message on the channel
         * when in this callback, if the channel will have flow turned on
         * and off repeatedly.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_flow_open(
            const std::string &channel_name,
            Channel *const channel_ptr) =0;

        /**
         * Called when the channel has been permanently closed (no new
         * items can be sent) and will not be reopened.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_flow_closed(
            const std::string &channel_name,
            Channel * const channel_ptr) =0;

        /**
         * Called when the channel instance has been destructed.  The pointer
         * will not be valid after this method returns.
         * @param channel_name[in] The channel name.
         * @param channel_ptr[in] Pointer to the actual channel, typically
         * for identification/lookup purposes.
         */
        virtual void channel_destructed(
            const std::string &channel_name,
            Channel * const channel_ptr) =0;
    };
}
}

#endif //MUTGOS_EVENTS_CHANNELCONTROLLISTENER_H
