Design Document
===============


Leaves
------

Leaf Node
~~~~~~~~~

``Leaf`` is the base class which contains NameInfo and SeqNo.

NameInfo
++++++++

``NameInfo`` is an abstract class and has only one implementation class ``StdNameInfo``.
Name is managed as a string in ``StdNameInfo``.

``StdNameInfo`` also maintains an internal table of all created ``StdNameInfo`` objects.
(**NOT CLEAR WHY**)

SeqNo
+++++

``SeqNo`` contains two pieces of information ``m_session`` and ``m_seq``.

``m_session`` (uint64) is Session ID (e.g., after crash, application will choose new session ID.
Session IDs for the same name should always increase.

``m_seq`` (uint64) is Sequence Number for a session.
It always starts with 0 and goes to max value.
(**For now, wrapping sequence number after max to zero is not supported**)

``SeqNo`` also contains a member ``m_valid``.
It seems that ``SeqNo`` is "invalid" if ``m_seq`` is zero.

Full Leaf Node
~~~~~~~~~~~~~~

``FullLeaf`` inherits from ``Leaf``.
Compared to ``Leaf``, ``FullLeaf`` can calculate digest of a leaf.

Digest is calculated as ``H(H(name)|H(session|seq))``.

Diff Leaf Node
~~~~~~~~~~~~~~

``DiffLeaf`` inherits from ``Leaf``.
Compared to ``Leaf``, ``DiffLeaf`` contains information about operation: Update & Remove.


State (Tree)
------------

State
~~~~~

``State`` is an abstract class of digest tree.
This class defines two methods ``update`` and ``remove``.
It has only one implementation class ``FullState``.

FullState
~~~~~~~~~

``FullState`` implements the interface defined by ``State`` and also support digest calculation.
Digest is calculated as:

0. If the tree is empty, the digest is directly set to ``00``.
1. Sort all leaves according to names (in std::string order, see std::string::compare).
2. ``H(H(l1)|H(l2)|...|H(ln))``



``FullState`` also records the timestamp of the last update.

DiffState
~~~~~~~~~

``DiffState`` can be considered as a "commitment", which includes a set of ``DiffLeaves``.
``DiffState`` is maintained as a list.
Each ``DiffState`` object has a pointer to the next diff.
Each ``DiffState`` object also has a digest which corresponds to its full state.


SyncLogic
---------

``SyncLogic`` is the core class of ChronoSync.

Initialization
~~~~~~~~~~~~~~

``SyncLogic`` requires a ``m_syncPrefix``, a callback ``m_onUpdate`` to handle notification of state update, and a callback ``m_onRemove`` to handle notification of leaf removal.

When created, ``SyncLogic`` will listen to ``m_syncPrefix`` to receive sync interest.
A ``m_syncInterestTable`` is created to hold incoming sync interest with the same state digest as the receiving instance.

``SyncLogic`` also has a ``m_perBranch`` boolean member and a ``m_onUpdateBranch`` callback member.
(**NOT CLEAR ABOUT THE USAGE**. It seems that if ``m_perBranch`` is set, ``m_onUpdateBranch`` will be called, but ``m_onRemove`` and ``m_onUpdate`` will not be called.)

Working Flow (onData)
~~~~~~~~~~~~~~~~~~~~~

``SyncLogic`` periodically expresses sync interest which contains its own state digest.
Sync interest is constructed by concatenating the ``m_syncPrefix`` with the hex-encoded state digest.
The ``MustBeFresh`` field of the sync interest must be set.
When a sync interest is sent out, a timer is set up to re-express sync interest.
The value of timer is set to ``4,000,000 + rand(100, 500)`` milliseconds.

Sync interest is associated with two callbacks: ``onSyncData`` and ``onSyncTimeout``.
``onSyncTimeout`` does nothing, because interest re-expressing is handled by a scheculer.
``onSyncData`` validates the data. Invalid data will be dropped.
If data can be validated, the packet will go through following process:

1. ``SyncLogic`` deletes interests in ``m_syncInterestTable`` which can be satisfied by the sync data, because these interests
in ``m_syncInterestTable`` must have been satisfied before the data packet is received.

2. ``SyncLogic`` decodes the sync data into a ``DiffState`` object. ``DiffLeaves`` in ``DiffState`` is sorted according to name.
``SyncLogic`` applies each ``DiffLeaf`` in the sorted order. If the operation in ``DiffLeaf`` changes the status, ``SyncLogic``
will generate its own diff log which is also a ``DiffState`` object. If ``SyncLogic`` detect that the received sync data has some
information that it is missing, it will prepare a ``MissingDataInfo`` which includes ``prefix`` (name), ``low`` (lowest missing SeqNo),
and ``high`` (highest missing SeqNo), and call ``m_onUpdate`` on it.
If the content of sync data cannot be parsed, the packet is dropped.

3. ``SyncLogic`` compares the sync data name with ``m_outstandingInterestName`` which is the name of the outstanding sync interest
sent by the instance. If so, ``SyncLogic`` expresses new sync interest after sync data processing is done, otherwise does nothing

Working Flow (onInterest)
~~~~~~~~~~~~~~~~~~~~~~~~~

``SyncLogic`` also expects to receive sync interest from others.
If a sync interest is received, the packet will be processed as following:

1. ``SyncLogic`` checks whether the interest is a normal interest or recovery interest (which will be described later).
If data is a normal sync data packet, go to Step 2, otherwise go to Step 7.

2. Receiving a normal sync interest, check if its digest is ``00``. If so, go to Step 3, otherwise go to Step 4.

3. Receiving an initial sync interest (sending instance has an empty sync tree). If receiving ``SyncLogic`` 's sync tree is not empty,
encode the whole sync tree into response (sync data), otherwise both sender and receiver are in the same status, put the interest into
``m_syncInterestTable``.

4. Receiving a sync interest which contains some state. Check if the sender and receiver are in the same status, if so, put the interest
into ``m_syncInterestTable``. Otherwise check if sender's state exist in receiver's log, if so, merge all the difference into one
``DiffState`` and encode it into response (sync data). If the receiver cannot recognize the sender's state, go to Step 5.

5. Check if immediate recovery is needed (whether ``timedProcessing`` is set), if so, express recovery interest immediately (go to Step 6).
Otherwise check if the interest exist in ``m_syncInterestTable``. If the interest has not been inserted into the table before,
process the interest again after ``rand(200, 100)`` milliseconds but with ``timedProcessing set``. If the interest exist in the table,
insert it again simply refreshes the table and defers the interest processing for another random period. It is guaranteed that a recovery
interest will be sent out when the interest is processed again.

6. Send recovery interest. A recovery interest is constructed by inserting a name component ``recovery`` between the ``m_syncPrefix`` and
the received digest. Default recovery interest re-expressing timer is 200 milliseconds with a jitter ``rand(100, 500)`` milliseconds.
Every time when recovery interest times out, the timer will be doubled until is longer than 100 seconds.

7. Receiving a recovery sync interest, check if its digest is recongized. If not, drop the interest, otherwise, encode the sync tree in the
response.

Name Publishing
~~~~~~~~~~~~~~~

``addLocalName`` method is used to update leaf nodes.
Caller needs to supply prefix (name), session, and seq.
The 3-tuple will trigger state changes and sync interest/data exchange.

Data Validation (in paper, not implemented)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each leaf as a sibling for endorsement.
The name of the sibling is constructed by concatenating the original leaf's name with a name component ``ENDORSE``.
``SyncLogic``, when parsing the returned data, will automatically fetch & process ``ENDORSE`` data.

Unresolved Issue
~~~~~~~~~~~~~~~~

1. Remove leaf

2. SeqNo wrapp-up

SyncSocket
----------

``SyncSocket`` is a wrapper of ``SyncLogic``. It can be viewed as an interface to a leaf node.
Through SyncSocket, user only needs to publish its data.
``SyncSocket`` will maitain corresponding sequence number, and trigger synchronization throug ``SyncLogic::addLocalName`` method.
``SyncSocket`` is also responsible for actual data publishing.
