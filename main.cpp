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

bool const Debug = false;

/*
 * Utils
 */

template< typename TKey, typename TValue >
bool contain( std::map< TKey, TValue > const & map, TKey const & key ){
    return map.find( key ) != map.end();
}

template< typename T >
bool contain( std::vector< T > const & array, T const & value ){
    return std::find( array.begin(), array.end(), value ) != array.end();
}

template< typename T > 
void test( T const & t ){
    using namespace std::chrono;

    time_point< system_clock, nanoseconds > const & start = high_resolution_clock::now();

    t(); 

    time_point< system_clock, nanoseconds > const & end = high_resolution_clock::now();

    duration< long int, std::nano > const & diff = end - start;

    std::cout
        << "> in "
        << duration_cast< microseconds >( diff ).count()
        << "µs"
        << std::endl;
}

/*
 * Node
 */

struct Node{
    Node() = default;
    Node( Node const & ) = delete;
    Node( Node && ) = delete;

    ~Node() = default;

    Node & operator=( Node const & ) = delete;
    Node & operator=( Node && ) = delete;

    void free(){
        for( auto const & pair : children_ ){
            if( pair.second != nullptr ){
                pair.second->free();
            }
        }

        delete this;
    }

    bool end_ = false;
    std::map< char, Node * > children_;
};

Node * getOrCreate( Node * const node, char const c ){
    Node *& result = node->children_[ c ];

    if( result != nullptr ){
        return result;
    }

    result = new Node;

    return result;
}

struct TrieStats{
    TrieStats( Node const * const root ){
        traverse( root );
    }

    void traverse( Node const * const node ){
        nodesCounter_ += 1;
        childrenCounter_ += node->children_.size();

        if( node->end_ ){
            wordsCounter_ += 1;
        }

        if( node->children_.empty() ){
            leavesCounter_ += 1;
        }
        else{
            if( node->children_.size() == 1 ){
                nodeWithOneChildCounter_ += 1;
            }

            for( auto const & pair : node->children_ ){
                traverse( pair.second );
            }
        }
    }

    unsigned nodesCounter_ = 0;
    unsigned leavesCounter_ = 0;
    unsigned childrenCounter_ = 0;
    unsigned wordsCounter_ = 0;
    unsigned nodeWithOneChildCounter_ = 0;
};

/*
 * Keyboard Layout
 */

std::string const polishKeyboardLayout(
    "|1|2|3|4|5|6|7|8|9|0|-|=|\n"
    " |q|w|e|r|t|y|u|i|o|p|[|\n"
    "  |a|s|d|f|g|h|j|k|l|;|'|\n"
    "   |z|x|c|v|b|n|m|,|.|/|"
);

std::string const polishKeyboardShiftLayout(
    "|!|@|#|$|%|^|&|*|(|)|_|+|\n"
    " |Q|W|E|R|T|Y|U|I|O|P|{|\n"
    "  |A|S|D|F|G|H|J|K|L|:|\"|\n"
    "   |Z|X|C|V|B|N|M|<|>|?|"
);

struct KeyboardLayout{
    struct Position{
        unsigned x_;
        unsigned y_;
        unsigned z_;

        unsigned distance( Position const & other ) const {
            unsigned const distance =
                abs( x_ - other.x_ ) +
                abs( y_ - other.y_ ) +
                abs( z_ - other.z_ );

            return distance;
        }
    };

    void addLayout( unsigned id, std::istream & input ){
        unsigned lineNo = 0;
        std::string line;

        while( std::getline( input, line ) ){
            char c = 0;
            unsigned i = 0;

            while( i < line.size() ){
                c = line[ i++ ];
                
                if( c == '|' ){
                    break;
                }
            }

            while( i < line.size() ){
                c = line[ i++ ];

                if( c != ' ' ){
                    if( layout_.count( c ) > 0 ){
                        throw std::runtime_error( "invalid keyboard layout (1)" );
                    }

                    layout_[ c ] = Position{ id, lineNo, i };
                }

                c = line[ i++ ];

                if( c != '|' ){
                    throw std::runtime_error( "invalid keyboard layout (2)" );
                }
            }


            lineNo += 1;
        }
    }

    unsigned distance( char c1, char c2 ) const {
        if( layout_.count( c1 ) == 0 ){
            return -1;
        }

        if( layout_.count( c2 ) == 0 ){
            return -1;
        }

        return layout_.find( c1 )->second.distance( layout_.find( c2 )->second );
    }

    std::map< char, Position > layout_;
};

/*
 * PenaltyPolicy
 */

struct PenaltyPolicy{
    PenaltyPolicy( KeyboardLayout const * keyboardLayout )
        : keyboardLayout_( keyboardLayout )
    
    {
    }

    virtual ~PenaltyPolicy(){
    }

    virtual int maxNumberOfMistakes( int const wordLength ) const {
        return std::max( 3, wordLength );
    }

    virtual int swapLetter( char const currentLetter, char const nextLetter ) const {
        return 2;
    }

    virtual int insertLetter( char const currentLetter, char const insertedLetter, char const nextLetter = char( 0 ) ) const {
        return 3;
    }

    virtual int replaceLetter( char const currentLetter, char const replaceLetter, char const nextLetter = char( 0 ) ) const {

        unsigned distance1 = keyboardLayout_->distance( currentLetter, replaceLetter );

        if( distance1 == -1 ){
            distance1 = 4;
        }

        if( distance1 > 4 ){
            distance1 = 4;
        }

        if( distance1 == 0 ){
            distance1 = 2;
        }

        if( nextLetter == char( 0 ) ){
            return distance1;
        }


        unsigned distance2 = keyboardLayout_->distance( replaceLetter, nextLetter );

        if( distance2 == -1 ){
            distance2 = 4;
        }

        if( distance2 > 4 ){
            distance2 = 4;
        }

        if( distance2 == 0 ){
            distance2 = 2;
        }

        return std::min( distance1, distance2 );
    }

    virtual int exactMatch( char const currentLetter = char( 0 ) ) const {
        return 0;
    }

    virtual int deleteLetter( char const previousLetter, char const currentLetter, char const nextLetter = char( 0 ) ) const {
        return 3;
    }

    KeyboardLayout const * keyboardLayout_;
};

/*
 * TrieIterator
 */

struct TrieIterator{
    TrieIterator(
        Node * const root,
        int const penalty,
        std::vector< TrieIterator * > & iterators,
        PenaltyPolicy * penaltyPolicy,
        std::string const & word,
        std::string const & debug

    )
        : iterators_( iterators )
        , penaltyPolicy_( penaltyPolicy )
        , penalty_( penalty )
        , node_( root )
        , word_( word )
        , debug_( debug )
    {
    }

    virtual void move( char const c, char const nextLetter = char( 0 ) );
    
    int getPenalty() const {
        return penalty_;
    }

    std::vector< TrieIterator * > & iterators_;
    PenaltyPolicy * penaltyPolicy_;
    int penalty_;
    Node * node_;
    std::string word_;
    std::string debug_;
};

struct SkipIteration
    : TrieIterator{

    SkipIteration(
        Node * const root,
        int const penalty,
        std::vector< TrieIterator * > & iterators,
        PenaltyPolicy * penaltyPolicy,
        std::string const & word,
        std::string const & debug
    )
        : TrieIterator(
            root,
            penalty,
            iterators,
            penaltyPolicy,
            word,
            debug )
        , skip_( true ){
    }

    void move( char const c, char const nextLetter ) override{
        if( skip_ ){
            skip_ = false;
            return;
        }

        TrieIterator::move( c, nextLetter );
    }

    bool skip_;
};

void TrieIterator::move( char const c, char const nextLetter ){
    if( nextLetter != char( 0 ) ){
        if( contain( node_->children_, nextLetter ) ){
            if( contain( node_->children_[ nextLetter ]->children_, c ) ){
                iterators_.push_back(
                    new SkipIteration(
                        node_->children_[ nextLetter ]->children_[ c ],
                        penalty_ + penaltyPolicy_->swapLetter( c, nextLetter ),
                        iterators_,
                        penaltyPolicy_,
                        word_ + std::string( 1, nextLetter ) + std::string( 1, c ),
                        debug_ + "S"
                    )
                );
            }
        }
    }

    for( auto const & pair : node_->children_ ){
        if( contain( pair.second->children_, c ) ){
            iterators_.push_back(
                new TrieIterator(
                    pair.second->children_[ c ],
                    penalty_ + penaltyPolicy_->insertLetter( c, pair.first, nextLetter ),
                    iterators_,
                    penaltyPolicy_,
                    word_ + std::string( 1, pair.first ) + std::string( 1, c ),
                    debug_ + "I"
                )
            );
        }
    }

    for( auto const & pair : node_->children_ ){
        if( pair.first == c ){
            iterators_.push_back(
                new TrieIterator(
                    node_->children_[ pair.first ],
                    penalty_ + penaltyPolicy_->exactMatch( pair.first ),
                    iterators_,
                    penaltyPolicy_,
                    word_ + std::string( 1, pair.first ),
                    debug_ + "E"
                )
            );
        }
        else{
            iterators_.push_back(
                new TrieIterator(
                    node_->children_[ pair.first ],
                    penalty_ + penaltyPolicy_->replaceLetter( c, pair.first, nextLetter ),
                    iterators_,
                    penaltyPolicy_,
                    word_ + std::string( 1, pair.first ),
                    debug_ + "R"
                )
            );
        }
    }

    char const previousLetter = word_.size() < 2 ? char( 0 ) : word_[ word_.size() - 2 ];
    penalty_ += penaltyPolicy_->deleteLetter( previousLetter, c, nextLetter );
    debug_ += "D";
}

/*
 * SpellCheckerBase
 */

struct SpellCheckerBase{
    typedef std::vector< TrieIterator * >::const_iterator iterator;
    typedef std::vector< TrieIterator * >::const_iterator const_iterator;

    SpellCheckerBase()
        : counter_( 0 )
        , trie_( new Node ){
    }

    SpellCheckerBase( SpellCheckerBase const & ) = delete;
    SpellCheckerBase( SpellCheckerBase && ) = delete;

    virtual ~SpellCheckerBase(){
        trie_->free();
    }

    SpellCheckerBase & operator=( SpellCheckerBase const & ) = delete;
    SpellCheckerBase & operator=( SpellCheckerBase && ) = delete;

    void init( PenaltyPolicy * penaltyPolicy ){
        counter_ = 0;

        penaltyPolicy_ = penaltyPolicy;

        iterators_.clear();
        iterators_.push_back(
            new TrieIterator(
                trie_,
                0,
                iterators_,
                penaltyPolicy_,
                "",
                ""
            )
        );
    }

    void finalize(){
        std::for_each(
            iterators_.begin(),
            iterators_.end(),
            []( TrieIterator const * const node ){ delete node; }
        );

        iterators_.clear();
    }

    void readDictFile( std::string const & fileName ){
        std::ifstream file( fileName.c_str() );
        
        if( ! file ){
            throw std::runtime_error( "Can't open file: " + fileName );
        }

        std::string line;
        
        while( std::getline( file, line ) ){
            Node * node = trie_;
            
            for( unsigned i = 0 ; i < line.size() ; ++ i ){
                node = getOrCreate( node, line[i] );

                if( i + 1 == line.size() ){
                    node->end_ = true;
                }
            }
        }
    }

    void processLetter( char const c, char const nextLetterHint = char(0) ){
        for( unsigned current = 0, end = iterators_.size() ; current != end ; ++ current ){
            iterators_[ current ]->move( c, nextLetterHint );
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

    iterator begin() const {
        return iterators_.begin();
    }

    iterator end() const {
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

struct SpellChecker : SpellCheckerBase{
    SpellChecker( std::string const & fileName ){
        readDictFile( fileName );

        std::istringstream iss1( polishKeyboardLayout );
        keyboardLayout_.addLayout( 1, iss1 );

        std::istringstream iss2( polishKeyboardShiftLayout );
        keyboardLayout_.addLayout( 0, iss2 );

        TrieStats ts( trie_ );

        if( Debug ){
            std::cout << "Nodes counter: " << ts.nodesCounter_ << std::endl;
            std::cout << "Leaves counter: " << ts.leavesCounter_ << std::endl;
            std::cout << "Avg. children/node: " << 1.0 * ts.childrenCounter_ / ts.nodesCounter_ << std::endl;
            std::cout << "Words counter: " << ts.wordsCounter_ << std::endl;
            std::cout << "Node with one child: " << ts.nodeWithOneChildCounter_ << std::endl;
        }
    }

    std::vector< std::string > getSuggestionsImpl( std::string const & word ){
        if( word.size() < 2 ){
            return std::vector< std::string >( 1, word );
        }

        PenaltyPolicy penaltyPolicy( & keyboardLayout_ );
        init( & penaltyPolicy );

        for( unsigned i = 1 ; i < word.size() ; ++ i ){
            processLetter( word[ i - 1 ], word[ i ] );

            if( Debug ){
                for( auto const & i : (*this) ){
                    std::cout << "> " << i->word_ << " " << i->debug_ << " " << i->penalty_ << std::endl;
                }
                std::cout << std::endl;
            }
        }

        processLetter( word[ word.size() - 1 ] );

        std::vector< TrieIterator * > iterators( begin(), end() );

        std::sort(
            iterators.begin(),
            iterators.end(),
            []( TrieIterator const * const lhs, TrieIterator const * const rhs ){
                return lhs->penalty_ < rhs->penalty_;
            }
        );

        std::vector< std::string > result;

        for( auto const & i : iterators ){
            if( i->node_->end_ ){

                if( Debug ){
                    std::cout << "> " << i->word_ << " " << i->debug_ << " " << i->penalty_ << std::endl;
                }

                if( contain( result, i->word_ ) == false ){
                    result.push_back( i->word_ );
                }
            }
        }

        finalize();

        return result;
    }

    std::vector< std::string > getSuggestions( std::string const & word ){
        std::vector< std::string > result;
        test( [ this, & word, & result ](){ result = this->getSuggestionsImpl( word ); } );
        return result;
    }

    KeyboardLayout keyboardLayout_;
};

/*
 * main
 */

int main( int args, char* argv[] ){
    SpellChecker sc( argv[ 1 ] );

    while( true ){
        std::string word;
        std::cout << "? ";
        std::cin >> word;

        for( std::string const & suggestion : sc.getSuggestions( word ) ){
            std::cout << suggestion << std::endl;
        }
    }

    return 0;
}

