#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

/*
 * Utils
 */

template< typename TKey, typename TValue >
bool contain( std::map< TKey, TValue > const & map, TKey const & key )
{
    return map.find( key ) != map.end();
}

template< typename T >
bool contain( std::vector< T > const & array, T const & value )
{
    return std::find( array.begin(), array.end(), value ) != array.end();
}

/*
 * Node
 */

struct Node
{
    void free()
    {
        for( auto pair : children_ )
        {
            if( pair.second != nullptr ){
                pair.second->free();
            }
        }

        delete this;
    }

    char letter_ = char( 0 );
    bool end_ = false;
    Node * parent_ = nullptr;
    std::map< char, Node * > children_;
};

Node * getOrCreate( Node * const node, char const c )
{
    Node *& result = node->children_[ c ];

    if( result != nullptr ){
        return result;
    }

    result = new Node;
    result->letter_ = c;
    result->parent_ = node;

    return result;
}

std::string getWord( Node const * const node )
{
    if( node == nullptr ){
        return "";
    }

    if( node->letter_ == char( 0 ) ){
        return getWord( node->parent_ );
    }
    else{
        return getWord( node->parent_ ) + std::string( 1, node->letter_ );
    }
}

struct TrieStats
{
    TrieStats( Node const * const root )
    {
        traverse( root );
    }

    void traverse( Node const * const node )
    {
        nodesCounter_ += 1;
        childrenCounter_ += node->children_.size();

        if( node->end_ ){
            wordsCounter_ += 1;
        }

        if( node->children_.empty() ){
            leavesCounter_ += 1;
        }
        else
        {
            for( auto const & pair : node->children_ ){
                traverse( pair.second );
            }
        }
    }

    unsigned nodesCounter_ = 0;
    unsigned leavesCounter_ = 0;
    unsigned childrenCounter_ = 0;
    unsigned wordsCounter_ = 0;
};

/*
 * Keyboard Layout
 */

std::string const polishKeyboardLayout(
    "| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 | - | = |\n"
    " | q | w | e | r | t | y | u | i | o | p | [ |\n"
    "  | a | s | d | f | g | h | j | k | l | ; | ' |\n"
    "   | z | x | c | v | b | n | m | , | . | / |"
);

std::string const polishKeyboardShiftLayout(
    "| ! | @ | # | $ | % | ^ | & | * | ( | ) | _ | + |\n"
    " | Q | W | E | R | T | Y | U | I | O | P | { |\n"
    "  | A | S | D | F | G | H | J | K | L | : | \" |\n"
    "   | Z | X | C | V | B | N | M | < | > | ? |"
);

std::string const polishKeyboardAltLayout(
    "|   |   |   |   |   |   |   |   |   |   |   |   |\n"
    " |   |   | ę |   |   |   |   |   | ó |   |   |\n"
    "  | ą | ś |   |   |   |   |   |   | ł |   |   |\n"
    "   | ż | ź | ć |   |   | ń |   |   |   |   |"
);

struct KeyboardLayout
{
    struct Position
    {
        unsigned x_;
        unsigned y_;
        unsigned z_;

        unsigned distance( Position const & other ) const
        {
            unsigned const distance =
                2 * abs( x_ - other.x_ ) +
                2 * abs( y_ - other.y_ ) +
                0.5 * abs( z_ - other.z_ );

            return std::min( 6u, distance );
        }
    };

    void addLayout( unsigned id, std::istream & input )
    {
        unsigned lineNo = 0;
        std::string line;
        while( std::getline( input, line ) )
        {
            char c = 0;
            unsigned i = 0;

            while( i < line.size() )
            {
                c = line[ i++ ];
                
                if( c == '|' ){
                    break;
                }
            }

            while( i < line.size() )
            {
                c = line[ i++ ];

                if( c != ' ' ){
                    throw std::runtime_error( "invalid keyboard layout" );
                }

                c = line[ i++ ];

                if( c != ' ' )
                {
                    if( layout_.count( c ) > 0 ){
                        throw std::runtime_error( "invalid keyboard layout" );
                    }

                    layout_[ c ] = Position{ id, lineNo, i };
                }

                c = line[ i++ ];

                if( c != ' ' ){
                    throw std::runtime_error( "invalid keyboard layout" );
                }

                c = line[ i++ ];

                if( c != '|' ){
                    throw std::runtime_error( "invalid keyboard layout" );
                }
            }


            lineNo += 1;
        }
    }

    unsigned distance( char c1, char c2 ) const
    {
        if( layout_.count( c1 ) == 0 ){
            return 8;
        }

        if( layout_.count( c2 ) == 0 ){
            return 8;
        }

        return layout_.find( c1 )->second.distance( layout_.find( c2 )->second );
    }

    std::map< char, Position > layout_;
};

/*
 * PenaltyPolicy
 */

struct PenaltyPolicy
{
    PenaltyPolicy( KeyboardLayout const * keyboard )
        : keyboard_( keyboard )
    {
    }

    virtual ~PenaltyPolicy()
    {
    }

    virtual int maxNumberOfMistakes( int const wordLength ) const
    {
        switch( wordLength )
        {
            case 1:
            case 2: 
            case 3:
                return 2;
            default:
                return wordLength;
        }
    }

    virtual int insertLetter( char const previousLetter, char const currentLetter, char const nextLetter = char( 0 ) ) const
    {
        if( nextLetter == char( 0 ) ){
            return keyboard_->distance( previousLetter, currentLetter );
        }

        return 
            std::min( 
                keyboard_->distance( previousLetter, currentLetter ),
                keyboard_->distance( currentLetter, nextLetter )
            );
    }

    virtual int replaceLetter( char const previousLetter, char const currentLetter, char const nextLetter = char( 0 ) ) const
    {
        return keyboard_->distance( previousLetter, currentLetter );
    }

    virtual int exactMatch( char const currentLetter = char( 0 ) ) const
    {
        return 0;
    }

    virtual int deleteLetter( char const previousLetter, char const currentLetter, char const nextLetter = char( 0 ) ) const
    {
        if( nextLetter == char( 0 ) ){
            return keyboard_->distance( previousLetter, currentLetter );
        }

        return 
            std::min( 
                keyboard_->distance( previousLetter, currentLetter ),
                keyboard_->distance( currentLetter, nextLetter )
            );
    }

    KeyboardLayout const * keyboard_;
};

/*
 * TrieIterator
 */

struct TrieIterator
{
    TrieIterator(
        Node * const root,
        int const penalty,
        std::vector< TrieIterator * > & iterators,
        PenaltyPolicy * penaltyPolicy

#ifndef NDEBUG
        , std::string const & debug
#endif

    )
        : iterators_( iterators )
        , penaltyPolicy_( penaltyPolicy )
        , penalty_( penalty )
        , node_( root )

#ifndef NDEBUG
        , debug_( debug )
#endif

    {
    }

    void move( char const c, char const nextLetter = char( 0 ) )
    {
        for( auto pair : node_->children_ )
        {
            if( contain( pair.second->children_, c ) )
            {
                iterators_.push_back(
                    new TrieIterator(
                        pair.second->children_[ c ],
                        penalty_ + penaltyPolicy_->insertLetter( c, pair.first, nextLetter ),
                        iterators_,
                        penaltyPolicy_
#ifndef NDEBUG
                        , debug_ + "I"
#endif
                    )
                );
            }
        }

        for( auto pair : node_->children_ )
        {
            if( pair.first == c )
            {
                iterators_.push_back(
                    new TrieIterator(
                        node_->children_[ pair.first ],
                        penalty_ + penaltyPolicy_->exactMatch( c ),
                        iterators_,
                        penaltyPolicy_
#ifndef NDEBUG
                        , debug_ + "E"
#endif
                    )
                );
            }
            else
            {
                iterators_.push_back(
                    new TrieIterator(
                        node_->children_[ pair.first ],
                        penalty_ + penaltyPolicy_->replaceLetter( c, pair.first, nextLetter ),
                        iterators_,
                        penaltyPolicy_
#ifndef NDEBUG
                        , debug_ + "R"
#endif
                    )
                );
            }
        }

        char const previousLetter = node_->parent_ ? node_->parent_->letter_ : char( 0 );
        penalty_ += penaltyPolicy_->deleteLetter( previousLetter, c, nextLetter );

#ifndef NDEBUG
        debug_ += "D";
#endif

    }
    
    int getPenalty() const
    {
        return penalty_;
    }

    std::vector< TrieIterator * > & iterators_;
    PenaltyPolicy * penaltyPolicy_;
    int penalty_;
    Node * node_;

#ifndef NDEBUG
    std::string debug_;
#endif
};

/*
 * SpellCheckerBase
 */

struct SpellCheckerBase
{
    typedef std::vector< TrieIterator * >::const_iterator iterator;
    typedef std::vector< TrieIterator * >::const_iterator const_iterator;

    SpellCheckerBase()
        : counter_( 0 )
        , trie_( new Node )
    {
    }

    SpellCheckerBase( SpellCheckerBase const & ) = delete;
    SpellCheckerBase( SpellCheckerBase && ) = delete;

    virtual ~SpellCheckerBase()
    {
        trie_->free();
    }

    SpellCheckerBase & operator=( SpellCheckerBase const & ) = delete;
    SpellCheckerBase & operator=( SpellCheckerBase && ) = delete;

    void init( PenaltyPolicy * penaltyPolicy )
    {
        counter_ = 0;

        penaltyPolicy_ = penaltyPolicy;

        iterators_.clear();
        iterators_.push_back(
            new TrieIterator(
                trie_,
                0,
                iterators_,
                penaltyPolicy_
#ifndef NDEBUG
                , ""
#endif
            )
        );
    }

    void finalize()
    {
        std::for_each(
            iterators_.begin(),
            iterators_.end(),
            []( TrieIterator const * const node ){ delete node; }
        );
    }

    void readDictFile( std::string const & fileName )
    {
        std::ifstream file( fileName.c_str() );
        
        if( ! file )
        {
            throw std::runtime_error( "Can't open file: " + fileName );
        }

        std::string line;
        
        while( std::getline( file, line ) )
        {
            Node * node = trie_;
            
            for( unsigned i = 0 ; i < line.size() ; ++ i )
            {
                node = getOrCreate( node, line[i] );

                if( i + 1 == line.size() ){
                    node->end_ = true;
                }
            }
        }
    }

    void emitLetter( char const c, char const nextLetterHint = char(0) )
    {
        for( unsigned current = 0, end = iterators_.size() ; current != end ; ++ current )
        {
            iterators_[ current ]->move( c );
        }

        //std::cout << "Iterator counter: " << iterators_.size() << std::endl;

        int const penalty = penaltyPolicy_->maxNumberOfMistakes( ++ counter_ );

        auto const toBeRemoved = std::partition(
            iterators_.begin(),
            iterators_.end(),
            [ penalty ]( TrieIterator const * const node ){ return node->getPenalty() <= penalty; }
        );

        std::for_each(
            toBeRemoved,
            iterators_.end(),
            []( TrieIterator const * const node ){ delete node; }
        );

        iterators_.erase( toBeRemoved, iterators_.end() );
    }

    iterator begin() const
    {
        return iterators_.begin();
    }

    iterator end() const
    {
        return iterators_.end();
    }

    unsigned counter_;
    Node * trie_;
    std::vector< TrieIterator * > iterators_;
    PenaltyPolicy * penaltyPolicy_;
};

/*
 * SpellChecker
 */

struct SpellChecker : SpellCheckerBase
{
    SpellChecker( std::string const & fileName )
    {
        readDictFile( fileName );

        std::istringstream iss1( polishKeyboardLayout );
        keyboard_.addLayout( 1, iss1 );

        std::istringstream iss2( polishKeyboardShiftLayout );
        keyboard_.addLayout( 0, iss2 );

        //std::istringstream iss3( polishKeyboardAltLayout );
        //keyboard_.addLayout( 2, iss3 );

#ifndef NDEBUG
        TrieStats ts( trie_ );
        std::cout << "Nodes counter: " << ts.nodesCounter_ << std::endl;
        std::cout << "Leaves counter: " << ts.leavesCounter_ << std::endl;
        std::cout << "Avg. children/node: " << 1.0 * ts.childrenCounter_ / ts.nodesCounter_ << std::endl;
        std::cout << "Words counter: " << ts.wordsCounter_ << std::endl;
#endif

    }

    std::vector< std::string > getSuggestions( std::string const & word )
    {
        PenaltyPolicy penaltyPolicy( & keyboard_ );
        init( & penaltyPolicy );

        for( char const c : word )
        {
            emitLetter( c );
        }

        std::vector< TrieIterator * > iterators( begin(), end() );

        std::sort(
            iterators.begin(),
            iterators.end(),
            []( TrieIterator const * const lhs, TrieIterator const * const rhs ){
                return lhs->penalty_ < rhs->penalty_;
            }
        );

        std::vector< std::string > result;

        for( auto i : iterators )
        {
            if( i->node_->end_ )
            {
                std::string suggestion = getWord( i->node_ );

#ifndef NDEBUG
                std::cout << "> " << suggestion << " " << i->penalty_ << " " << i->debug_ << std::endl;
#endif

                if( contain( result, suggestion ) == false ){
                    result.push_back( std::move( suggestion ) );
                }
            }
        }

        finalize();

        return result;
    }

    KeyboardLayout keyboard_;
};

/*
 * main
 */

int main( int args, char* argv[] )
{
    SpellChecker sc( argv[ 1 ] );

    while( true )
    {
        std::string word;
        std::cout << "? ";
        std::cin >> word;

        for( std::string const & suggestion : sc.getSuggestions( word ) )
        {
            std::cout << suggestion << std::endl;
        }
    }

    return 0;
}

