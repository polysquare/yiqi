/*
 * commandline.cpp:
 * Definitions of a function to extract a command line
 * from the currently running program argv and also the
 * instrumentation parameters
 *
 * See LICENCE.md for Copyright information
 */

#include <algorithm>
#include <stdexcept>
#include <vector>

#include <cstring>

#include <assert.h>

#include "commandline.h"
#include "instrumentation_tool.h"
#include "scopeguard.h"

namespace ycom = yiqi::commandline;
namespace yit = yiqi::instrumentation::tools;

ycom::CommandArguments
ycom::BuildCommandLine (int                 argc,
                        char const * const  *argv,
                        yit::Program const  &tool)
{
    if (argc < 1)
        throw std::runtime_error ("provided argc must have at "
                                  "least the prorgram name");

    /* In the usual case we will have three args */
    ycom::CommandArguments arguments;
    arguments.reserve (3);

    /* Instrumentation tool */
    std::string const &wrapper (tool.WrapperBinary ());

    if (!wrapper.empty ())
        arguments.push_back (wrapper);

    /* Tool arguments */
    std::string const &toolOption (tool.WrapperOptions ());

    if (!toolOption.empty ())
        arguments.push_back (toolOption);

    /* The name of this test binary */
    arguments.push_back (argv[0]);

    return arguments;
}

ycom::ArgvVector const
ycom::StringVectorToArgv (CommandArguments const &args)
{
    ycom::ArgvVector argv;

    for (auto const &arg : args)
        argv.push_back (arg.c_str ());

    argv.push_back (nullptr);

    return argv;
}

namespace yiqi
{
    namespace commandline
    {
        class NullTermArray::Private
        {
            public:

                Private ();
                explicit Private (char const * const *array);
                Private (Private const &);
                Private & operator= (Private rhs);

                bool operator== (Private const &rhs) const;
                bool operator!= (Private const &lhs) const;

                friend void swap (Private &lhs, Private &rhs);

                std::vector <char const *> vector;
                std::vector <std::string>  storedNewStrings;

            private:

                static size_t arrayLength (char const * const *);
        };

        void swap (NullTermArray::Private &lhs,
                   NullTermArray::Private &rhs);
    }
}

size_t
ycom::NullTermArray::Private::arrayLength (char const * const *array)
{
    size_t size = 0;
    for (; array[size] != NULL; ++size);
    return size + 1;
}

ycom::NullTermArray::Private::Private ()
{
    /* Always start with a null-terminator */
    vector.push_back (NULL);
}

ycom::NullTermArray::Private::Private (char const * const *array) :
    vector (array, array + arrayLength (array))
{
}

ycom::NullTermArray::Private::Private (Private const &priv) :
    vector (priv.vector)
{
}

void
ycom::swap (NullTermArray::Private &lhs,
            NullTermArray::Private &rhs)
{
    using std::swap;

    swap (lhs.vector, rhs.vector);
}

ycom::NullTermArray::Private &
ycom::NullTermArray::Private::operator= (Private rhs)
{
    using ycom::swap;

    swap (*this, rhs);
    return *this;
}

bool
ycom::NullTermArray::Private::operator== (Private const &rhs) const
{
    return vector == rhs.vector;
}

bool
ycom::NullTermArray::Private::operator!= (Private const &rhs) const
{
    return !(*this == rhs);
}

ycom::NullTermArray::NullTermArray () :
    priv (new Private ())
{
}

ycom::NullTermArray::~NullTermArray ()
{
}

ycom::NullTermArray::NullTermArray (char const * const *envp) :
    priv (new Private (envp))
{
}

ycom::NullTermArray::NullTermArray (NullTermArray const &env) :
    priv (new Private (*env.priv))
{
}

ycom::NullTermArray::NullTermArray (NullTermArray &&env) :
    priv (std::move (env.priv))
{
}

void
ycom::swap (NullTermArray &lhs, NullTermArray &rhs)
{
    using std::swap;
    swap (*lhs.priv, *rhs.priv);
}

ycom::NullTermArray &
ycom::NullTermArray::operator= (NullTermArray rhs)
{
    swap (*this, rhs);
    return *this;
}

bool
ycom::NullTermArray::operator== (NullTermArray const &rhs) const
{
    /* Either is null */
    if ((!priv && rhs.priv) || (priv && !rhs.priv))
        return false;

    NullTermArray::Private const &pref = *priv;
    NullTermArray::Private const &rhsref = *rhs.priv;
    return pref == rhsref;
}

bool
ycom::NullTermArray::operator!= (NullTermArray const &lhs) const
{
    return !(*this == lhs);
}

void
ycom::NullTermArray::append (std::string const &value)
{
    std::vector <std::string> vec =
    {
        value
    };

    append (vec);
}

void
ycom::NullTermArray::removeAnyMatching (RemoveFunc const &remover)
{
    auto wrapper = [this, &remover](char const *str) -> bool {
        if (str)
            return remover (str);
        else
            return false;
    };

    priv->vector.erase (std::remove_if (priv->vector.begin (),
                                        priv->vector.end (),
                                        wrapper),
                        priv->vector.end ());
}

namespace
{
    template <typename HSIter, typename Needles>
    struct Pred
    {
        typedef typename Needles::const_iterator NIter;
        typedef std::function <bool (HSIter const &, NIter const &)> Func;
    };

    template <typename HSIter, typename N>
    HSIter
    rangeMatchingPredicate (N                                     &values,
                            HSIter                          const &first,
                            HSIter                          const &end,
                            typename Pred <HSIter, N>::Func const &pred)
    {
        auto last = first;

        /* Keep going until either the end, or until the block
         * where we've inserted stops. We already have the first
         * iterator, so move directly to the second */
        for (typename N::const_iterator it (values.begin () + 1);
             it != values.end ();
             ++it)
        {
            ++last;

            /* Exhausted first list, found the last matching
             * iterator there that could match this list */
            if (last == end)
                break;

            /* Found the first iterator that does not match */
            if (!pred (last, it))
                break;
        }

        /* We need to put the last iterator at
         * one past the position we intend to erase */
        ++last;

        return last;
    }
}

void
ycom::NullTermArray::eraseAppended (StringVector const &values)
{
    typedef typename StringVector::iterator SVIterator;

    SVIterator storedNewStringsEnd = priv->storedNewStrings.end ();
    /* Start search from at least values.size () from the end */
    auto searchStartPoint = storedNewStringsEnd -
                            (values.size ());
    SVIterator firstStoredNewStringsIterator =
        std::find (searchStartPoint,
                   storedNewStringsEnd,
                   values.front ());

    if (firstStoredNewStringsIterator != storedNewStringsEnd)
    {
        typedef typename StringVector::const_iterator CSVIterator;

        auto stringsEqual =
            [](SVIterator const &lhs, CSVIterator const &rhs) -> bool {
                return *lhs == *rhs;
            };

        auto lastStoredNewStringIterator =
            rangeMatchingPredicate (values,
                                    firstStoredNewStringsIterator,
                                    storedNewStringsEnd,
                                    stringsEqual);

        typedef std::vector <char const *>::iterator CVIterator;

        /* Handle the duplicate-pointers edge case by
         * starting from end - distance (first, last) - 1 */
        CVIterator vectorEnd = priv->vector.end ();
        auto searchStartPoint =
            vectorEnd - std::distance (firstStoredNewStringsIterator,
                                       lastStoredNewStringIterator) - 1;

        auto pointerInVectorMatchingRawStringPointer =
            [&firstStoredNewStringsIterator](char const *str) -> bool {
                /* We want to compare the pointers, as it was pointers
                 * that were inserted, not new values */
                return firstStoredNewStringsIterator->c_str () == str;
            };

        CVIterator firstPointerInVector =
            std::find_if (searchStartPoint,
                          vectorEnd,
                          pointerInVectorMatchingRawStringPointer);

        if (firstPointerInVector != vectorEnd)
        {
            auto stringEqualsCharacterArray =
                [](CVIterator const &lhs, CSVIterator const &rhs) -> bool {
                    return *lhs == rhs->c_str ();
                };

            auto lastPointerInVector =
                rangeMatchingPredicate (values,
                                        firstPointerInVector,
                                        vectorEnd,
                                        stringEqualsCharacterArray);

            /* Erase this from the vector of pointers */
            priv->vector.erase (firstPointerInVector,
                                lastPointerInVector);
        }

        /* Erase this block from the vector of stored
         * new strings */
        priv->storedNewStrings.erase (firstStoredNewStringsIterator,
                                      lastStoredNewStringIterator);
    }
}

void
ycom::NullTermArray::append (std::vector <std::string> const &vec)
{
    auto cleanup (yiqi::ScopeGuard (
                      std::bind (&ycom::NullTermArray::eraseAppended,
                                 this,
                                 vec)));

    /* Reserve some more space */
    size_t const oldLength = priv->storedNewStrings.size ();
    size_t const oldAllocation = priv->storedNewStrings.capacity ();

    priv->storedNewStrings.insert (priv->storedNewStrings.end (),
                                   vec.begin (),
                                   vec.end ());

    auto firstNewlyStoredString (priv->storedNewStrings.begin ());

    /* If the new minimum size would require a reallocation, then
     * we need to take the pointer of every member of storedNewStrings.
     * If a reallocation doesn't happen then we can skip that step */
    if (oldAllocation > oldLength + vec.size ())
        std::advance (firstNewlyStoredString, oldLength);

    size_t const requiredSize = priv->vector.size () + vec.size ();

    priv->vector.resize (requiredSize);

    size_t const startIndexForVector =
        (requiredSize - 1) -
        priv->storedNewStrings.size () +
        std::distance (priv->storedNewStrings.begin (),
                       firstNewlyStoredString);

    size_t vectorIndex = startIndexForVector;

    for (auto it = firstNewlyStoredString;
         it != priv->storedNewStrings.end ();
         ++it, ++vectorIndex)
    {
        priv->vector[vectorIndex] = it->c_str ();
    }

    /* The last should always be a null terminator */
    assert (*(priv->vector.end () - 1) == NULL);

    cleanup.dismiss ();
}

void
ycom::InsertEnvironmentPair (NullTermArray &array,
                             char const    *key,
                             char const    *value)
{
    std::string const KeyEqValue (std::string (key) +
                                  "=" +
                                  std::string (value));
    array.append (KeyEqValue);
}

char const * const *
ycom::NullTermArray::underlyingArray () const
{
    return &(priv->vector[0]);
}

size_t
ycom::NullTermArray::underlyingArrayLen () const
{
    return priv->vector.size ();
}
