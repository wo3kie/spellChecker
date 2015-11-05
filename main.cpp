#include <algorithm>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <memory>
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
 * PenaltyPolicy
 */

struct PenaltyPolicy
{
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
                return 1;
            case 4: 
            case 5: 
                return 2;
            case 6:
            default:
                return 3;
        }
    }

    virtual int insertLetter( char const previousLetter, char const currentLetter, char const nextLetter = char( 0 ) ) const
    {
        return 1;
    }

    virtual int replaceLetter( char const previousLetter, char const currentLetter, char const nextLetter = char( 0 ) ) const
    {
        return 1;
    }

    virtual int deleteLetter( char const previousLetter, char const currentLetter, char const nextLetter = char( 0 ) ) const
    {
        return 1;
    }
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
            if( contain( pair.second->children_, c ) ){
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
            if( pair.first == c ){
                iterators_.push_back(
                    new TrieIterator(
                        node_->children_[ pair.first ],
                        penalty_,
                        iterators_,
                        penaltyPolicy_
#ifndef NDEBUG
                        , debug_ + "E"
#endif
                    )
                );
            }
            else{
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

        penalty_ += penaltyPolicy_->deleteLetter( char( 0 ), c, nextLetter );

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
        PenaltyPolicy penaltyPolicy;
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
};

/*
 * main
 */

int main( int args, char* argv[] )
{
    SpellChecker sc( argv[ 1 ] );

    for( std::string const & word : sc.getSuggestions( argv[ 2 ] ) )
    {
        std::cout << word << std::endl;
    }

    return 0;
}

