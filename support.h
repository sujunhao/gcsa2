/*
  Copyright (c) 2015 Genome Research Ltd.

  Author: Jouni Siren <jouni.siren@iki.fi>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef _GCSA_SUPPORT_H
#define _GCSA_SUPPORT_H

#include "utils.h"

namespace gcsa
{

//------------------------------------------------------------------------------

template<class ByteVector>
void characterCounts(const ByteVector& sequence, const sdsl::int_vector<8>& char2comp, sdsl::int_vector<64>& counts);

/*
  This replaces the SDSL byte_alphabet. The main improvements are:
    - The alphabet can be built from an existing sequence.
    - The comp order does not need to be the same as character order, as long as \0 is the first character.
*/

class Alphabet
{
public:
  typedef gcsa::size_type size_type;
  const static size_type MAX_SIGMA = 256;

  const static sdsl::int_vector<8> DEFAULT_CHAR2COMP;
  const static sdsl::int_vector<8> DEFAULT_COMP2CHAR;

  Alphabet();
  Alphabet(const Alphabet& a);
  Alphabet(Alphabet&& a);
  ~Alphabet();

  /*
    ByteVector only has to support operator[] and size(). If there is a clearly faster way for
    sequential access, function characterCounts() should be specialized.
  */
  template<class ByteVector>
  explicit Alphabet(const ByteVector& sequence,
    const sdsl::int_vector<8>& _char2comp = DEFAULT_CHAR2COMP,
    const sdsl::int_vector<8>& _comp2char = DEFAULT_COMP2CHAR) :
    char2comp(_char2comp), comp2char(_comp2char),
    C(sdsl::int_vector<64>(_comp2char.size() + 1, 0)),
    sigma(_comp2char.size())
  {
    if(sequence.size() == 0) { return; }

    characterCounts(sequence, this->char2comp, this->C);
    for(size_type i = 0, sum = 0; i < this->C.size(); i++)
    {
      size_type temp = this->C[i]; this->C[i] = sum; sum += temp;
    }
  }

  /*
    The counts array holds character counts for all comp values.
  */
  explicit Alphabet(const sdsl::int_vector<64>& counts,
    const sdsl::int_vector<8>& _char2comp = DEFAULT_CHAR2COMP,
    const sdsl::int_vector<8>& _comp2char = DEFAULT_COMP2CHAR);

  void swap(Alphabet& a);
  Alphabet& operator=(const Alphabet& a);
  Alphabet& operator=(Alphabet&& a);

  size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const;
  void load(std::istream& in);

  sdsl::int_vector<8>  char2comp, comp2char;
  sdsl::int_vector<64> C;
  size_type            sigma;

private:
  void copy(const Alphabet& a);
};  // class Alphabet

template<class ByteVector>
void
characterCounts(const ByteVector& sequence, const sdsl::int_vector<8>& char2comp, sdsl::int_vector<64>& counts)
{
  for(size_type c = 0; c < counts.size(); c++) { counts[c] = 0; }
  for(size_type i = 0; i < sequence.size(); i++) { counts[char2comp[sequence[i]]]++; }
}

//------------------------------------------------------------------------------

/*
  This interface is intended for indexing kmers of length 16 or less on an alphabet of size
  8 or less. The kmer is encoded as an 64-bit integer (most significant bit first):
    - 16x3 bits for the kmer, with high-order characters 0s when necessary
    - 8 bits for marking which predecessors are present
    - 8 bits for marking which successors are present
*/

typedef std::uint64_t key_type;

struct Key
{
  const static size_type CHAR_WIDTH = 3;
  const static size_type CHAR_MASK = 0x7;
  const static size_type MAX_LENGTH = 16;

  inline static key_type encode(const Alphabet& alpha, const std::string& label,
    byte_type pred, byte_type succ)
  {
    key_type value = 0;
    for(size_type i = 0; i < label.length(); i++)
    {
      value = (value << CHAR_WIDTH) | alpha.char2comp[label[i]];
    }
    value = (value << 8) | pred;
    value = (value << 8) | succ;
    return value;
  }

  static std::string decode(key_type key, size_type kmer_length, const Alphabet& alpha);

  inline static size_type kmer(key_type key) { return (key >> 16); }
  inline static byte_type predecessors(key_type key) { return (key >> 8) & 0xFF; }
  inline static byte_type successors(key_type key) { return key & 0xFF; }
  inline static comp_type last(key_type key) { return (key >> 16) & CHAR_MASK; }

  inline static key_type merge(key_type key1, key_type key2) { return (key1 | (key2 & 0xFFFF)); }
  inline static key_type replace(key_type key, size_type kmer_val)
  {
    return (kmer_val << 16) | (key & 0xFFFF);
  }
};

//------------------------------------------------------------------------------

typedef std::uint64_t node_type;

struct Node
{
  const static size_type OFFSET_BITS = 8;
  const static size_type OFFSET_MASK = 0xFF;

  inline static node_type encode(size_type node_id, size_type node_offset)
  {
    return (node_id << OFFSET_BITS) | node_offset;
  }

  static node_type encode(const std::string& token);
  static std::string decode(node_type node);

  inline static size_type id(node_type node) { return node >> OFFSET_BITS; }
  inline static size_type offset(node_type node) { return node & OFFSET_MASK; }
};

//------------------------------------------------------------------------------

struct KMer
{
  key_type  key;
  node_type from, to;

  KMer();
  KMer(const std::vector<std::string>& tokens, const Alphabet& alpha, size_type successor);

  inline bool
  operator< (const KMer& another) const
  {
    return (Key::kmer(this->key) < Key::kmer(another.key));
  }

  inline bool sorted() const { return (this->from == this->to); }
  inline void makeSorted() { this->to = this->from; }

  static byte_type chars(const std::string& token, const Alphabet& alpha);
};

std::ostream& operator<< (std::ostream& out, const KMer& kmer);

inline bool
operator< (key_type key, const KMer& kmer)
{
  return (Key::kmer(key) < Key::kmer(kmer.key));
}

/*
  This function does several things:

  1. Sorts the kmer array by the kmer labels encoded in the key
  2. Builds an array of unique kmer labels, with the predecessor and successor
     fields merged from the original kmers.
  3. Stores the last character of each unique kmer label in an array.
  4. Replaces the kmer labels in the keys by their ranks.
  5. Marks the kmers sorted if they have unique labels.
*/
void uniqueKeys(std::vector<KMer>& kmers, std::vector<key_type>& keys, sdsl::int_vector<0>& last_char,
  bool print = false);

//------------------------------------------------------------------------------

/*
  The node type used during doubling. As in the original GCSA, from and to are nodes
  in the original graph, denoting a path as a semiopen range [from, to). If
  from == to, the path will not be extended, because it already has a unique label.
  rank_type is the integer type used to store ranks of the original kmers.
  During edge generation, to will be used to store the number of outgoing edges.
*/

struct PathNode
{
  typedef std::uint32_t rank_type;

  const static size_type LABEL_LENGTH = 8;

  node_type from, to;
  rank_type label[LABEL_LENGTH];
  size_type fields; // Lowest 8 bits for predecessors, next 8 bits for order.

  inline bool sorted() const { return (this->from == this->to); }
  inline void makeSorted() { this->to = this->from; }

  inline byte_type order() const { return ((this->fields >> 8) & 0xFF); }
  inline void setOrder(byte_type new_order)
  {
    this->fields &= ~(size_type)0xFF00;
    this->fields |= ((size_type)new_order) << 8;
  }

  inline byte_type predecessors() const { return (this->fields & 0xFF); }
  inline void setPredecessors(byte_type preds)
  {
    this->fields &= ~(size_type)0xFF;
    this->fields |= (size_type)preds;
  }

  inline void addPredecessors(const PathNode& another)
  {
    this->fields |= another.predecessors();
  }

  inline bool hasPredecessor(comp_type comp) const
  {
    return (this->fields & (1 << comp));
  }

  inline size_type outdegree() const { return this->to; }

  explicit PathNode(const KMer& kmer);
  PathNode(const PathNode& left, const PathNode& right);
  explicit PathNode(std::ifstream& in);

  size_type serialize(std::ostream& out) const;

  PathNode();
  PathNode(const PathNode& source);
  PathNode(PathNode&& source);
  ~PathNode();

  void copy(const PathNode& source);
  void swap(PathNode& another);

  PathNode& operator= (const PathNode& source);
  PathNode& operator= (PathNode&& source);
};

struct PathLabelComparator
{
  size_type max_length;

  PathLabelComparator(size_type len = PathNode::LABEL_LENGTH) : max_length(len) {}

  inline bool operator() (const PathNode& a, const PathNode& b) const
  {
    for(size_t i = 0; i < this->max_length; i++)
    {
      if(a.label[i] != b.label[i]) { return (a.label[i] < b.label[i]); }
    }
    return false;
  }
};

struct PathFromComparator
{
  inline bool operator() (const PathNode& a, const PathNode& b) const
  {
    return (a.from < b.from);
  }
};

//------------------------------------------------------------------------------

} // namespace gcsa

#endif // _GCSA_SUPPORT_H
